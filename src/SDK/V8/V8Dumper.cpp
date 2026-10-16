#include "V8Dumper.h"

#include "SDK/Files/Paths.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Panorama/UIEngine.h"
#include "SDK/V8/V8.h"

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
    constexpr const char* DumpFileName = "panorama_dump.txt";

    std::FILE* Output = nullptr;
    std::size_t LineCount = 0;

    void PrintAddress(std::uintptr_t Address)
    {
        HMODULE Module = nullptr;
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(Address), &Module))
        {
            std::fprintf(Output, "%#llx", static_cast<unsigned long long>(Address));
            return;
        }

        char Path[MAX_PATH] = {};
        GetModuleFileNameA(Module, Path, MAX_PATH);
        const char* Name = std::strrchr(Path, '\\');
        Name = Name ? Name + 1 : Path;
        std::fprintf(Output, "%s+%#llx", Name, static_cast<unsigned long long>(Address - reinterpret_cast<std::uintptr_t>(Module)));
    }

    void PrintNative(V8::Local Function)
    {
        V8::NativeInfo Info;
        if (!V8::GetNativeInfo(Function, Info))
            return;

        if (!Info.Api)
        {
            std::fputs(" [js]", Output);
            return;
        }

        std::fputs(" -> ", Output);
        PrintAddress(Info.Callback);

        if (Info.Method & 1)
            std::fprintf(Output, " method=vtable+%#llx", static_cast<unsigned long long>(Info.Method - 1));
        else if (Info.Method)
        {
            std::fputs(" method=", Output);
            PrintAddress(Info.Method);
        }

        if (Info.Adjust)
            std::fprintf(Output, " this+%#llx", static_cast<unsigned long long>(Info.Adjust));
    }

    void DumpObject(void* Isolate, V8::Local Context, V8::Local Object, int Depth, int MaxDepth)
    {
        V8::Local Names = V8::GetPropertyNames(Object, Context);
        if (!Names)
            return;

        std::uint32_t Count = V8::ArrayLength(Names);
        for (std::uint32_t Index = 0; Index < Count; ++Index)
        {
            V8::Local Key = V8::GetIndex(Names, Context, Index);
            if (!Key)
                continue;

            std::string Name = V8::ToString(Isolate, Key);
            V8::Local Value = V8::Get(Object, Context, Key);

            std::fprintf(Output, "%*s%s : ", Depth * 4, "", Name.c_str());
            ++LineCount;

            if (!Value)
            {
                std::fputs("<exception>\n", Output);
                continue;
            }

            bool FunctionValue = V8::IsFunction(Value);
            bool ObjectValue = !FunctionValue && V8::IsObject(Value);

            std::fputs(V8::TypeOf(Isolate, Value).c_str(), Output);
            if (FunctionValue)
                PrintNative(Value);
            else if (!ObjectValue)
                std::fprintf(Output, " = %s", V8::ToString(Isolate, Value).c_str());
            std::fputc('\n', Output);

            if ((FunctionValue || ObjectValue) && Depth < MaxDepth)
                DumpObject(Isolate, Context, Value, Depth + 1, MaxDepth);
        }
    }

    void DumpGlobalContext(void* Isolate, V8::Local Context)
    {
        V8::ContextEnter(Context);

        std::fputs("=== Global context ===\n", Output);
        ++LineCount;
        DumpObject(Isolate, Context, V8::ContextGlobal(Context), 0, 1);

        V8::ContextExit(Context);
    }

    void DumpPanelContext(void* Isolate, V8::Local Context)
    {
        V8::ContextEnter(Context);

        V8::Local Global = V8::ContextGlobal(Context);
        V8::Local Dollar = V8::Get(Isolate, Global, Context, "$");
        V8::Local Getter = Dollar ? V8::Get(Isolate, Dollar, Context, "GetContextPanel") : nullptr;
        V8::Local Panel = Getter && V8::IsFunction(Getter) ? V8::Call(Getter, Context, Dollar, 0, nullptr) : nullptr;

        if (Panel && V8::IsObject(Panel))
        {
            V8::Local Id = V8::Get(Isolate, Panel, Context, "id");
            std::fprintf(Output, "=== Panel context (%s) ===\n", Id ? V8::ToString(Isolate, Id).c_str() : "?");
            ++LineCount;
            DumpObject(Isolate, Context, Panel, 0, 0);
        }
        else
            LogW("GetContextPanel failed");

        V8::ContextExit(Context);
    }

    V8::Local FindPanelContext(void* Isolate, const UIEngine* Engine)
    {
        std::int32_t Count = Engine->PanelContextCount();
        for (std::int32_t Index = 0; Index < Count; ++Index)
        {
            V8::Address* Slot = Engine->PanelContext(Index);
            if (Slot && *Slot)
                return V8::CreateHandle(Isolate, *Slot);
        }
        return nullptr;
    }

    void DumpRegistry(const UIEngine* Engine)
    {
        std::int32_t ScopeCount = Engine->DocScopeCount();
        std::fprintf(Output, "=== Registry (%d scopes) ===\n", ScopeCount);
        ++LineCount;

        for (std::int32_t ScopeIndex = 0; ScopeIndex < ScopeCount; ++ScopeIndex)
        {
            const DocScope* Scope = Engine->Scope(ScopeIndex);
            std::int32_t FunctionCount = Scope->FunctionCount();
            std::fprintf(Output, "%s (%d)\n", Scope->Name(), FunctionCount);
            ++LineCount;

            for (std::int32_t FunctionIndex = 0; FunctionIndex < FunctionCount; ++FunctionIndex)
            {
                const DocFunction* Function = Scope->Function(FunctionIndex);
                std::fprintf(Output, "    %s", Function->Name());

                std::uint8_t ArgCount = Function->ArgCount();
                if (ArgCount != 0xFF)
                {
                    std::fputc('(', Output);
                    for (std::uint8_t Arg = 0; Arg < ArgCount; ++Arg)
                        std::fprintf(Output, "%s%u", Arg ? ", " : "", Function->ArgType(Arg));
                    std::fputc(')', Output);
                }

                std::fprintf(Output, " kind=%d ret=%u", Function->Kind(), Function->ReturnType());
                if (*Function->Description())
                    std::fprintf(Output, "  // %s", Function->Description());
                std::fputc('\n', Output);
                ++LineCount;
            }
        }
    }
}

void V8Dumper::Run()
{
    UIEngine* Engine = UIEngine::Get();
    if (!Engine)
    {
        LogE("UIEngine is null");
        return;
    }

    std::string DumpPath = Paths::Resolve(DumpFileName);
    Output = std::fopen(DumpPath.c_str(), "w");
    if (!Output)
    {
        LogE("Failed to open %s", DumpPath.c_str());
        return;
    }
    LineCount = 0;

    void* Isolate = Engine->Isolate();
    V8::Address* MainSlot = Engine->MainContext();

    V8::IsolateEnter(Isolate);
    V8::HandleScope Scope;
    V8::OpenHandleScope(Scope, Isolate);
    V8::TryCatch Catch;
    V8::OpenTryCatch(Catch, Isolate);

    if (MainSlot && *MainSlot)
        DumpGlobalContext(Isolate, V8::CreateHandle(Isolate, *MainSlot));
    else
        LogW("Main context not found");

    if (V8::Local PanelContext = FindPanelContext(Isolate, Engine))
        DumpPanelContext(Isolate, PanelContext);
    else
        LogW("Panel context not found");

    V8::CloseTryCatch(Catch);
    V8::CloseHandleScope(Scope);
    V8::IsolateExit(Isolate);

    DumpRegistry(Engine);

    std::fclose(Output);
    Output = nullptr;
    Log("Dump %zu lines -> %s", LineCount, DumpPath.c_str());
}
