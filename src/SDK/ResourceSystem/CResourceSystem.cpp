#include "CResourceSystem.h"

#include "Misc/Offsets.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <windows.h>

namespace
{
    constexpr const char* InterfaceName = "ResourceSystem013";
    constexpr std::uint8_t DefaultPriority = 0xFF;

    struct ManifestDesc
    {
        std::int32_t Count;
        const char** Paths;
        std::uint8_t Blocking;
        std::uint8_t Track;
        const char* DebugName;
        std::uint8_t Priority;
        void* Callback;
        void* Context;
    };

    CResourceSystem* Instance = nullptr;

    template <typename Fn>
    Fn Virtual(const void* Object, std::size_t Index)
    {
        return reinterpret_cast<Fn>(Memory::Read<void**>(reinterpret_cast<std::uintptr_t>(Object))[Index]);
    }
}

bool CResourceSystem::Init()
{
    HMODULE Module = GetModuleHandleA("resourcesystem.dll");
    auto CreateInterface = Module ? reinterpret_cast<void* (*)(const char*, int*)>(GetProcAddress(Module, "CreateInterface")) : nullptr;
    if (!CreateInterface)
    {
        LogE("resourcesystem.dll CreateInterface not found");
        return false;
    }
    Instance = static_cast<CResourceSystem*>(CreateInterface(InterfaceName, nullptr));
    if (!Instance)
    {
        LogE("%s not found", InterfaceName);
        return false;
    }
    Log("%s %p", InterfaceName, static_cast<void*>(Instance));
    return true;
}

CResourceSystem* CResourceSystem::Get()
{
    return Instance;
}

CResourceManifest* CResourceSystem::Load(const std::vector<const char*>& Paths, const char* DebugName, bool Blocking)
{
    if (Paths.empty())
        return nullptr;
    ManifestDesc Desc {};
    Desc.Count = static_cast<std::int32_t>(Paths.size());
    Desc.Paths = const_cast<const char**>(Paths.data());
    Desc.Blocking = Blocking ? 1 : 0;
    Desc.Track = 1;
    Desc.DebugName = DebugName;
    Desc.Priority = DefaultPriority;
    return Virtual<CResourceManifest* (*)(CResourceSystem*, ManifestDesc*)>(this, Offsets::CResourceSystem::CreateManifestIndex)(this, &Desc);
}

void CResourceSystem::Release(CResourceManifest* Manifest)
{
    if (Manifest)
        Virtual<void (*)(CResourceSystem*, CResourceManifest*)>(this, Offsets::CResourceSystem::ReleaseManifestIndex)(this, Manifest);
}

CResourceSystem::EStatus CResourceSystem::Status(std::uint64_t ResourceId)
{
    return static_cast<EStatus>(Virtual<std::int32_t (*)(CResourceSystem*, std::uint64_t)>(this, Offsets::CResourceSystem::StatusByIdIndex)(this, ResourceId));
}

const char* CResourceSystem::Name(std::uint64_t ResourceId)
{
    if (!ResourceId || Status(ResourceId) == EStatus::Absent)
        return nullptr;
    void* Binding = Virtual<void* (*)(CResourceSystem*, std::uint64_t, std::uint64_t)>(this, Offsets::CResourceSystem::FindBindingIndex)(this, ResourceId, 0);
    if (!Binding)
        return nullptr;
    auto* NameEntry = Memory::Read<const char**>(reinterpret_cast<std::uintptr_t>(Binding) + Offsets::ResourceBinding::Name);
    return NameEntry ? *NameEntry : nullptr;
}

void* CResourceSystem::Data(std::uint64_t ResourceId)
{
    if (!ResourceId || Status(ResourceId) != EStatus::Loaded)
        return nullptr;
    void* Binding = Virtual<void* (*)(CResourceSystem*, std::uint64_t, std::uint64_t)>(this, Offsets::CResourceSystem::FindBindingIndex)(this, ResourceId, 0);
    return Binding ? Memory::Read<void*>(reinterpret_cast<std::uintptr_t>(Binding)) : nullptr;
}
