#include "V8.h"

#include "Misc/Offsets.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <windows.h>

namespace
{
    struct Utf8Value
    {
        char* Text;
        int Length;
    };

    struct MaybeDouble
    {
        bool HasValue;
        double Value;
    };

    void (*IsolateEnterFn)(void*) = nullptr;
    void (*IsolateExitFn)(void*) = nullptr;
    void* (*HandleScopeCtor)(void*, void*) = nullptr;
    void (*HandleScopeDtor)(void*) = nullptr;
    V8::Address* (*CreateHandleFn)(void*, V8::Address) = nullptr;
    void* (*TryCatchCtor)(void*, void*) = nullptr;
    void (*TryCatchDtor)(void*) = nullptr;
    void (*ContextEnterFn)(V8::Local) = nullptr;
    void (*ContextExitFn)(V8::Local) = nullptr;
    V8::Local* (*ContextGlobalFn)(V8::Local, V8::Local*) = nullptr;
    V8::Local* (*NewFromUtf8Fn)(V8::Local*, void*, const char*, int, int) = nullptr;
    V8::Local* (*GetPropertyNamesFn)(V8::Local, V8::Local*, V8::Local) = nullptr;
    std::uint32_t (*ArrayLengthFn)(V8::Local) = nullptr;
    V8::Local* (*GetIndexFn)(V8::Local, V8::Local*, V8::Local, std::uint32_t) = nullptr;
    V8::Local* (*GetFn)(V8::Local, V8::Local*, V8::Local, V8::Local) = nullptr;
    V8::Local* (*CallFn)(V8::Local, V8::Local*, V8::Local, V8::Local, int, V8::Local*) = nullptr;
    bool (*IsFunctionFn)(V8::Local) = nullptr;
    bool (*IsObjectFn)(V8::Local) = nullptr;
    V8::Local* (*TypeOfFn)(V8::Local, V8::Local*, void*) = nullptr;
    Utf8Value* (*Utf8ValueCtor)(Utf8Value*, void*, V8::Local) = nullptr;
    void (*Utf8ValueDtor)(Utf8Value*) = nullptr;
    V8::Local* (*NumberNewFn)(V8::Local*, void*, double) = nullptr;
    V8::Local* (*ExternalNewFn)(V8::Local*, void*, void*) = nullptr;
    V8::Local* (*FunctionTemplateNewFn)(V8::Local*, void*, V8::Callback, V8::Local, V8::Local, int, int, int, const void*, std::uint16_t, std::uint16_t, std::uint16_t) = nullptr;
    V8::Local* (*GetFunctionFn)(V8::Local, V8::Local*, V8::Local) = nullptr;
    std::uint16_t* (*SetFn)(V8::Local, std::uint16_t*, V8::Local, V8::Local, V8::Local) = nullptr;
    V8::Address* (*GlobalizeFn)(void*, V8::Address) = nullptr;
    void (*DisposeGlobalFn)(V8::Address*) = nullptr;
    bool (*BooleanValueFn)(V8::Local, void*) = nullptr;
    MaybeDouble* (*NumberValueFn)(V8::Local, MaybeDouble*, V8::Local) = nullptr;
    bool (*HasCaughtFn)(const void*) = nullptr;
    V8::Local* (*ExceptionFn)(const void*, V8::Local*) = nullptr;
    void (*ResetFn)(void*) = nullptr;
    int (*InternalFieldCountFn)(V8::Local) = nullptr;
    V8::Local* (*GetInternalFieldFn)(V8::Local, V8::Local*, int) = nullptr;
    void* (*ExternalValueFn)(V8::Local) = nullptr;

    bool IsHeapObject(V8::Address Value)
    {
        return (Value & 3) == 1;
    }

    V8::Address Field(V8::Address Tagged, std::ptrdiff_t Offset)
    {
        return Memory::Read<V8::Address>(Tagged - 1 + Offset);
    }

    std::uint16_t InstanceType(V8::Address Tagged)
    {
        V8::Address Map = Memory::Read<V8::Address>(Tagged - 1);
        return Memory::Read<std::uint16_t>(Map - 1 + Offsets::V8::MapInstanceType);
    }

    bool HasType(V8::Address Value, std::uint16_t Type)
    {
        return IsHeapObject(Value) && InstanceType(Value) == Type;
    }

    bool ReadExternal(V8::Address Value, std::uintptr_t& Out)
    {
        if (!HasType(Value, Offsets::V8Type::JSExternalObject))
            return false;
        Out = Field(Value, Offsets::V8::ExternalValue);
        return true;
    }

    void ReadMethod(V8::Address Data, V8::NativeInfo& Out)
    {
        if (ReadExternal(Data, Out.Method))
            return;

        if (!IsHeapObject(Data) || InstanceType(Data) < Offsets::V8Type::JSObjectFirst)
            return;

        V8::Address Elements = Field(Data, Offsets::V8::JSObjectElements);
        if (!IsHeapObject(Elements))
            return;

        std::int64_t Length = static_cast<std::int64_t>(Field(Elements, Offsets::V8::FixedArrayLength)) >> 32;
        if (Length < 1 || !ReadExternal(Field(Elements, Offsets::V8::FixedArrayData), Out.Method))
            return;
        if (Length >= 2)
            ReadExternal(Field(Elements, Offsets::V8::FixedArrayData + 8), Out.Adjust);
    }

    template <typename T>
    bool Resolve(HMODULE Library, T& Target, const char* Symbol)
    {
        Target = reinterpret_cast<T>(reinterpret_cast<void*>(GetProcAddress(Library, Symbol)));
        if (!Target)
            LogE("V8 export not found: %s", Symbol);
        return Target != nullptr;
    }
}

bool V8::Init()
{
    HMODULE Library = GetModuleHandleA("v8.dll");
    if (!Library)
    {
        LogE("Module not loaded: v8.dll");
        return false;
    }

    bool Ok = true;
    Ok &= Resolve(Library, IsolateEnterFn, "?Enter@Isolate@v8@@QEAAXXZ");
    Ok &= Resolve(Library, IsolateExitFn, "?Exit@Isolate@v8@@QEAAXXZ");
    Ok &= Resolve(Library, HandleScopeCtor, "??0HandleScope@v8@@QEAA@PEAVIsolate@1@@Z");
    Ok &= Resolve(Library, HandleScopeDtor, "??1HandleScope@v8@@QEAA@XZ");
    Ok &= Resolve(Library, CreateHandleFn, "?CreateHandle@HandleScope@v8@@KAPEA_KPEAVIsolate@internal@2@_K@Z");
    Ok &= Resolve(Library, TryCatchCtor, "??0TryCatch@v8@@QEAA@PEAVIsolate@1@@Z");
    Ok &= Resolve(Library, TryCatchDtor, "??1TryCatch@v8@@QEAA@XZ");
    Ok &= Resolve(Library, ContextEnterFn, "?Enter@Context@v8@@QEAAXXZ");
    Ok &= Resolve(Library, ContextExitFn, "?Exit@Context@v8@@QEAAXXZ");
    Ok &= Resolve(Library, ContextGlobalFn, "?Global@Context@v8@@QEAA?AV?$Local@VObject@v8@@@2@XZ");
    Ok &= Resolve(Library, NewFromUtf8Fn, "?NewFromUtf8@String@v8@@SA?AV?$MaybeLocal@VString@v8@@@2@PEAVIsolate@2@PEBDW4NewStringType@2@H@Z");
    Ok &= Resolve(Library, GetPropertyNamesFn, "?GetPropertyNames@Object@v8@@QEAA?AV?$MaybeLocal@VArray@v8@@@2@V?$Local@VContext@v8@@@2@@Z");
    Ok &= Resolve(Library, ArrayLengthFn, "?Length@Array@v8@@QEBAIXZ");
    Ok &= Resolve(Library, GetIndexFn, "?Get@Object@v8@@QEAA?AV?$MaybeLocal@VValue@v8@@@2@V?$Local@VContext@v8@@@2@I@Z");
    Ok &= Resolve(Library, GetFn, "?Get@Object@v8@@QEAA?AV?$MaybeLocal@VValue@v8@@@2@V?$Local@VContext@v8@@@2@V?$Local@VValue@v8@@@2@@Z");
    Ok &= Resolve(Library, CallFn, "?Call@Function@v8@@QEAA?AV?$MaybeLocal@VValue@v8@@@2@V?$Local@VContext@v8@@@2@V?$Local@VValue@v8@@@2@HQEAV52@@Z");
    Ok &= Resolve(Library, IsFunctionFn, "?IsFunction@Value@v8@@QEBA_NXZ");
    Ok &= Resolve(Library, IsObjectFn, "?IsObject@Value@v8@@QEBA_NXZ");
    Ok &= Resolve(Library, TypeOfFn, "?TypeOf@Value@v8@@QEAA?AV?$Local@VString@v8@@@2@PEAVIsolate@2@@Z");
    Ok &= Resolve(Library, Utf8ValueCtor, "??0Utf8Value@String@v8@@QEAA@PEAVIsolate@2@V?$Local@VValue@v8@@@2@@Z");
    Ok &= Resolve(Library, Utf8ValueDtor, "??1Utf8Value@String@v8@@QEAA@XZ");
    Ok &= Resolve(Library, NumberNewFn, "?New@Number@v8@@SA?AV?$Local@VNumber@v8@@@2@PEAVIsolate@2@N@Z");
    Ok &= Resolve(Library, ExternalNewFn, "?New@External@v8@@SA?AV?$Local@VExternal@v8@@@2@PEAVIsolate@2@PEAX@Z");
    Ok &= Resolve(Library, FunctionTemplateNewFn, "?New@FunctionTemplate@v8@@SA?AV?$Local@VFunctionTemplate@v8@@@2@PEAVIsolate@2@P6AXAEBV?$FunctionCallbackInfo@VValue@v8@@@2@@ZV?$Local@VValue@v8@@@2@V?$Local@VSignature@v8@@@2@HW4ConstructorBehavior@2@W4SideEffectType@2@PEBVCFunction@2@GGG@Z");
    Ok &= Resolve(Library, GetFunctionFn, "?GetFunction@FunctionTemplate@v8@@QEAA?AV?$MaybeLocal@VFunction@v8@@@2@V?$Local@VContext@v8@@@2@@Z");
    Ok &= Resolve(Library, SetFn, "?Set@Object@v8@@QEAA?AV?$Maybe@_N@2@V?$Local@VContext@v8@@@2@V?$Local@VValue@v8@@@2@1@Z");
    Ok &= Resolve(Library, GlobalizeFn, "?GlobalizeReference@api_internal@v8@@YAPEA_KPEAVIsolate@internal@2@_K@Z");
    Ok &= Resolve(Library, DisposeGlobalFn, "?DisposeGlobal@api_internal@v8@@YAXPEA_K@Z");
    Ok &= Resolve(Library, BooleanValueFn, "?BooleanValue@Value@v8@@QEBA_NPEAVIsolate@2@@Z");
    Ok &= Resolve(Library, NumberValueFn, "?NumberValue@Value@v8@@QEBA?AV?$Maybe@N@2@V?$Local@VContext@v8@@@2@@Z");
    Ok &= Resolve(Library, HasCaughtFn, "?HasCaught@TryCatch@v8@@QEBA_NXZ");
    Ok &= Resolve(Library, ExceptionFn, "?Exception@TryCatch@v8@@QEBA?AV?$Local@VValue@v8@@@2@XZ");
    Ok &= Resolve(Library, ResetFn, "?Reset@TryCatch@v8@@QEAAXXZ");
    Ok &= Resolve(Library, InternalFieldCountFn, "?InternalFieldCount@Object@v8@@QEBAHXZ");
    Ok &= Resolve(Library, GetInternalFieldFn, "?GetInternalField@Object@v8@@QEAA?AV?$Local@VData@v8@@@2@H@Z");
    Ok &= Resolve(Library, ExternalValueFn, "?Value@External@v8@@QEBAPEAXXZ");

    return Ok;
}

void V8::IsolateEnter(void* Isolate)
{
    IsolateEnterFn(Isolate);
}

void V8::IsolateExit(void* Isolate)
{
    IsolateExitFn(Isolate);
}

void V8::OpenHandleScope(HandleScope& Scope, void* Isolate)
{
    HandleScopeCtor(Scope.Storage, Isolate);
}

void V8::CloseHandleScope(HandleScope& Scope)
{
    HandleScopeDtor(Scope.Storage);
}

void V8::OpenTryCatch(TryCatch& Catch, void* Isolate)
{
    TryCatchCtor(Catch.Storage, Isolate);
}

void V8::CloseTryCatch(TryCatch& Catch)
{
    TryCatchDtor(Catch.Storage);
}

V8::Local V8::CreateHandle(void* Isolate, Address Value)
{
    return CreateHandleFn(Isolate, Value);
}

void V8::ContextEnter(Local Context)
{
    ContextEnterFn(Context);
}

void V8::ContextExit(Local Context)
{
    ContextExitFn(Context);
}

V8::Local V8::ContextGlobal(Local Context)
{
    Local Result = nullptr;
    ContextGlobalFn(Context, &Result);
    return Result;
}

V8::Local V8::NewString(void* Isolate, const char* Text)
{
    Local Result = nullptr;
    NewFromUtf8Fn(&Result, Isolate, Text, 0, -1);
    return Result;
}

V8::Local V8::GetPropertyNames(Local Object, Local Context)
{
    Local Result = nullptr;
    GetPropertyNamesFn(Object, &Result, Context);
    return Result;
}

std::uint32_t V8::ArrayLength(Local Array)
{
    return ArrayLengthFn(Array);
}

V8::Local V8::GetIndex(Local Object, Local Context, std::uint32_t Index)
{
    Local Result = nullptr;
    GetIndexFn(Object, &Result, Context, Index);
    return Result;
}

V8::Local V8::Get(Local Object, Local Context, Local Key)
{
    Local Result = nullptr;
    GetFn(Object, &Result, Context, Key);
    return Result;
}

V8::Local V8::Get(void* Isolate, Local Object, Local Context, const char* Key)
{
    Local Name = NewString(Isolate, Key);
    if (!Name)
        return nullptr;
    return Get(Object, Context, Name);
}

V8::Local V8::Call(Local Function, Local Context, Local Receiver, int Argc, Local* Argv)
{
    Local Result = nullptr;
    CallFn(Function, &Result, Context, Receiver, Argc, Argv);
    return Result;
}

bool V8::IsFunction(Local Value)
{
    return IsFunctionFn(Value);
}

bool V8::IsObject(Local Value)
{
    return IsObjectFn(Value);
}

std::string V8::ToString(void* Isolate, Local Value)
{
    Utf8Value Text {};
    Utf8ValueCtor(&Text, Isolate, Value);
    std::string Result = Text.Text ? std::string(Text.Text, Text.Length) : std::string("<?>");
    Utf8ValueDtor(&Text);
    return Result;
}

std::string V8::TypeOf(void* Isolate, Local Value)
{
    Local Type = nullptr;
    TypeOfFn(Value, &Type, Isolate);
    return Type ? ToString(Isolate, Type) : std::string("<?>");
}

bool V8::GetNativeInfo(Local Function, NativeInfo& Out)
{
    Out = {};

    Address Tagged = *Function;
    if (!IsHeapObject(Tagged))
        return false;

    std::uint16_t Type = InstanceType(Tagged);
    if (Type < Offsets::V8Type::JSFunctionFirst || Type > Offsets::V8Type::JSFunctionLast)
        return false;

    Address Shared = Field(Tagged, Offsets::V8::JSFunctionShared);
    if (!HasType(Shared, Offsets::V8Type::SharedFunctionInfo))
        return false;

    Address Data = Field(Shared, Offsets::V8::SharedFunctionData);
    if (!HasType(Data, Offsets::V8Type::FunctionTemplateInfo))
        return true;

    Address CallCode = Field(Data, Offsets::V8::FunctionTemplateCallCode);
    if (!HasType(CallCode, Offsets::V8Type::CallHandlerInfo))
        return true;

    Out.Api = true;
    Out.Callback = Field(CallCode, Offsets::V8::CallHandlerCallback);
    ReadMethod(Field(CallCode, Offsets::V8::CallHandlerData), Out);
    return true;
}

V8::Local V8::NewNumber(void* Isolate, double Value)
{
    Local Result = nullptr;
    NumberNewFn(&Result, Isolate, Value);
    return Result;
}

V8::Local V8::NewFunction(void* Isolate, Local Context, Callback Function, void* Data)
{
    Local External = nullptr;
    ExternalNewFn(&External, Isolate, Data);

    Local Template = nullptr;
    FunctionTemplateNewFn(&Template, Isolate, Function, External, nullptr, 0, 1, 0, nullptr, 0, 0, 0);
    if (!Template)
        return nullptr;

    Local Result = nullptr;
    GetFunctionFn(Template, &Result, Context);
    return Result;
}

bool V8::Set(Local Object, Local Context, Local Key, Local Value)
{
    std::uint16_t Result = 0;
    SetFn(Object, &Result, Context, Key, Value);
    return (Result & 0xFF) && (Result >> 8);
}

V8::Address* V8::Globalize(void* Isolate, Local Value)
{
    return GlobalizeFn(Isolate, *Value);
}

void V8::DisposeGlobal(Address* Handle)
{
    DisposeGlobalFn(Handle);
}

bool V8::BooleanValue(void* Isolate, Local Value)
{
    return BooleanValueFn(Value, Isolate);
}

double V8::NumberValue(Local Value, Local Context)
{
    MaybeDouble Result {};
    NumberValueFn(Value, &Result, Context);
    return Result.HasValue ? Result.Value : 0.0;
}

void* V8::CallbackData(const void* Info)
{
    auto* ImplicitArgs = Memory::Read<Address*>(reinterpret_cast<std::uintptr_t>(Info) + Offsets::FunctionCallbackInfo::ImplicitArgs);
    std::uintptr_t Data = 0;
    if (!ReadExternal(ImplicitArgs[Offsets::FunctionCallbackInfo::DataIndex], Data))
        return nullptr;
    return reinterpret_cast<void*>(Data);
}

bool V8::HasCaught(const TryCatch& Catch)
{
    return HasCaughtFn(Catch.Storage);
}

V8::Local V8::Exception(const TryCatch& Catch)
{
    Local Result = nullptr;
    ExceptionFn(Catch.Storage, &Result);
    return Result;
}

void V8::ResetTryCatch(TryCatch& Catch)
{
    ResetFn(Catch.Storage);
}

int V8::InternalFieldCount(Local Object)
{
    return InternalFieldCountFn(Object);
}

V8::Local V8::GetInternalField(Local Object, int Index)
{
    Local Result = nullptr;
    GetInternalFieldFn(Object, &Result, Index);
    return Result;
}

void* V8::ExternalValue(Local Value)
{
    if (!Value || !HasType(*Value, Offsets::V8Type::JSExternalObject))
        return nullptr;
    return ExternalValueFn(Value);
}
