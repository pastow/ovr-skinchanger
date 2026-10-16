#include "UIEngine.h"

#include "Misc/Offsets.h"
#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

namespace
{
    UIEngine** EngineGlobal = nullptr;

    std::uintptr_t Self(const void* Object)
    {
        return reinterpret_cast<std::uintptr_t>(Object);
    }

    const char* SafeString(const char* Text)
    {
        return Text ? Text : "";
    }
}

const char* DocFunction::Name() const
{
    return SafeString(Memory::Read<const char*>(Self(this) + Offsets::DocFunction::Name));
}

const char* DocFunction::Description() const
{
    return SafeString(Memory::Read<const char*>(Self(this) + Offsets::DocFunction::Description));
}

std::int32_t DocFunction::Kind() const
{
    return Memory::Read<std::int32_t>(Self(this) + Offsets::DocFunction::Kind);
}

std::uint8_t DocFunction::ReturnType() const
{
    return Memory::Read<std::uint8_t>(Self(this) + Offsets::DocFunction::ReturnType);
}

std::uint8_t DocFunction::ArgCount() const
{
    return Memory::Read<std::uint8_t>(Self(this) + Offsets::DocFunction::ArgCount);
}

std::uint8_t DocFunction::ArgType(std::size_t Index) const
{
    if (Index >= Offsets::DocFunction::MaxArgs)
        return 0;
    return Memory::Read<std::uint8_t>(Self(this) + Offsets::DocFunction::ArgTypes + Index);
}

const char* DocScope::Name() const
{
    return SafeString(Memory::Read<const char*>(Self(this) + Offsets::DocScope::Name));
}

const char* DocScope::Description() const
{
    return SafeString(Memory::Read<const char*>(Self(this) + Offsets::DocScope::Description));
}

std::int32_t DocScope::FunctionCount() const
{
    return Memory::Read<std::int32_t>(Self(this) + Offsets::DocScope::FunctionCount);
}

const DocFunction* DocScope::Function(std::int32_t Index) const
{
    std::uintptr_t Functions = Memory::Read<std::uintptr_t>(Self(this) + Offsets::DocScope::Functions);
    return reinterpret_cast<const DocFunction*>(Functions + Index * Offsets::DocFunction::Size);
}

bool UIEngine::Init()
{
    std::uintptr_t Address = Memory::FindPattern("panorama.dll", Patterns::UIEngine);
    if (!Address)
        return false;

    EngineGlobal = reinterpret_cast<UIEngine**>(Memory::ResolveRelative(Address, 3, 7));
    Log("UIEngine %p -> %p", static_cast<void*>(EngineGlobal), static_cast<void*>(*EngineGlobal));
    return true;
}

UIEngine* UIEngine::Get()
{
    return EngineGlobal ? *EngineGlobal : nullptr;
}

void* UIEngine::Isolate() const
{
    return Memory::Read<void*>(Self(this) + Offsets::UIEngine::Isolate);
}

V8::Address* UIEngine::MainContext() const
{
    return Memory::Read<V8::Address*>(Self(this) + Offsets::UIEngine::MainContext);
}

std::int32_t UIEngine::PanelContextCount() const
{
    std::int32_t Count = Memory::Read<std::int32_t>(Self(this) + Offsets::UIEngine::PanelContextsCount);
    std::int32_t Capacity = Memory::Read<std::int32_t>(Self(this) + Offsets::UIEngine::PanelContextsCapacity) & 0x7FFFFFFF;
    return Count < Capacity ? Count : Capacity;
}

V8::Address* UIEngine::PanelContext(std::int32_t Index) const
{
    std::uintptr_t Entries = Memory::Read<std::uintptr_t>(Self(this) + Offsets::UIEngine::PanelContexts);
    std::uintptr_t Entry = Entries + Index * Offsets::PanelContextEntry::Size;
    if (Memory::Read<std::int32_t>(Entry + Offsets::PanelContextEntry::Index) == Index)
        return nullptr;

    std::uintptr_t Global = Memory::Read<std::uintptr_t>(Entry + Offsets::PanelContextEntry::Context);
    return Global ? Memory::Read<V8::Address*>(Global) : nullptr;
}

std::int32_t UIEngine::DocScopeCount() const
{
    return Memory::Read<std::int32_t>(Self(this) + Offsets::UIEngine::DocScopeCount);
}

const DocScope* UIEngine::Scope(std::int32_t Index) const
{
    std::uintptr_t Scopes = Memory::Read<std::uintptr_t>(Self(this) + Offsets::UIEngine::DocScopes);
    return reinterpret_cast<const DocScope*>(Scopes + Index * Offsets::DocScope::Size);
}
