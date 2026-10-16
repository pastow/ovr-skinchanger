#include "CSchemaSystem.h"

#include "Misc/Offsets.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <windows.h>

namespace
{
    CSchemaSystem* Instance = nullptr;

    std::uintptr_t Self(const void* Object)
    {
        return reinterpret_cast<std::uintptr_t>(Object);
    }

    const char* SafeString(const char* Text)
    {
        return Text ? Text : "";
    }
}

const char* CSchemaType::Name() const
{
    return SafeString(Memory::Read<const char*>(Self(this) + Offsets::CSchemaType::Name));
}

const char* SchemaClassFieldData::Name() const
{
    return SafeString(Memory::Read<const char*>(Self(this) + Offsets::SchemaClassFieldData::Name));
}

CSchemaType* SchemaClassFieldData::Type() const
{
    return Memory::Read<CSchemaType*>(Self(this) + Offsets::SchemaClassFieldData::Type);
}

std::int32_t SchemaClassFieldData::Offset() const
{
    return Memory::Read<std::int32_t>(Self(this) + Offsets::SchemaClassFieldData::Offset);
}

const char* CSchemaClassBinding::Name() const
{
    return SafeString(Memory::Read<const char*>(Self(this) + Offsets::CSchemaClassBinding::Name));
}

const char* CSchemaClassBinding::Module() const
{
    return SafeString(Memory::Read<const char*>(Self(this) + Offsets::CSchemaClassBinding::Module));
}

std::int32_t CSchemaClassBinding::Size() const
{
    return Memory::Read<std::int32_t>(Self(this) + Offsets::CSchemaClassBinding::Size);
}

std::int16_t CSchemaClassBinding::FieldCount() const
{
    return Memory::Read<std::int16_t>(Self(this) + Offsets::CSchemaClassBinding::FieldCount);
}

const SchemaClassFieldData* CSchemaClassBinding::Field(std::int32_t Index) const
{
    std::uintptr_t Fields = Memory::Read<std::uintptr_t>(Self(this) + Offsets::CSchemaClassBinding::Fields);
    if (!Fields)
        return nullptr;
    return reinterpret_cast<const SchemaClassFieldData*>(Fields + Index * Offsets::SchemaClassFieldData::Size);
}

CSchemaClassBinding* CSchemaClassBinding::BaseClass() const
{
    std::uintptr_t Base = Memory::Read<std::uintptr_t>(Self(this) + Offsets::CSchemaClassBinding::BaseClass);
    if (!Base)
        return nullptr;
    return Memory::Read<CSchemaClassBinding*>(Base + Offsets::CSchemaClassBinding::BaseClassBinding);
}

const char* CSchemaSystemTypeScope::Name() const
{
    return reinterpret_cast<const char*>(Self(this) + Offsets::CSchemaSystemTypeScope::Name);
}

std::vector<CSchemaClassBinding*> CSchemaSystemTypeScope::Classes() const
{
    std::vector<CSchemaClassBinding*> Result;
    std::uintptr_t Container = Self(this) + Offsets::CSchemaSystemTypeScope::ClassContainer;

    for (std::size_t Index = 0; Index < Offsets::CSchemaList::BlockCount; ++Index)
    {
        std::uintptr_t Block = Memory::Read<std::uintptr_t>(Container + Index * Offsets::CSchemaList::BlockContainerSize + Offsets::CSchemaList::FirstBlock);
        for (; Block; Block = Memory::Read<std::uintptr_t>(Block + Offsets::CSchemaList::NextBlock))
        {
            auto* Binding = Memory::Read<CSchemaClassBinding*>(Block + Offsets::CSchemaList::Binding);
            if (Binding)
                Result.push_back(Binding);
        }
    }
    return Result;
}

bool CSchemaSystem::Init()
{
    HMODULE Library = GetModuleHandleA("schemasystem.dll");
    if (!Library)
    {
        LogE("Module not loaded: schemasystem.dll");
        return false;
    }

    using CreateInterfaceFn = void* (*)(const char*, int*);
    auto CreateInterface = reinterpret_cast<CreateInterfaceFn>(reinterpret_cast<void*>(GetProcAddress(Library, "CreateInterface")));
    if (!CreateInterface)
    {
        LogE("Schemasystem CreateInterface not found");
        return false;
    }

    int ReturnCode = 0;
    Instance = static_cast<CSchemaSystem*>(CreateInterface("SchemaSystem_001", &ReturnCode));
    if (!Instance)
    {
        LogE("SchemaSystem_001 not found (%d)", ReturnCode);
        return false;
    }

    Log("SchemaSystem_001 %p", static_cast<void*>(Instance));
    return true;
}

CSchemaSystem* CSchemaSystem::Get()
{
    return Instance;
}

CSchemaSystemTypeScope* CSchemaSystem::GlobalTypeScope()
{
    using Fn = CSchemaSystemTypeScope* (*)(CSchemaSystem*);
    auto** VTable = *reinterpret_cast<void***>(this);
    return reinterpret_cast<Fn>(VTable[Offsets::CSchemaSystem::GlobalTypeScopeIndex])(this);
}

std::int32_t CSchemaSystem::TypeScopeCount() const
{
    return Memory::Read<std::uint16_t>(Self(this) + Offsets::CSchemaSystem::TypeScopeCount);
}

CSchemaSystemTypeScope* CSchemaSystem::TypeScope(std::int32_t Index) const
{
    std::uintptr_t Scopes = Memory::Read<std::uintptr_t>(Self(this) + Offsets::CSchemaSystem::TypeScopes);
    if (!Scopes)
        return nullptr;
    return Memory::Read<CSchemaSystemTypeScope*>(Scopes + Index * sizeof(void*));
}
