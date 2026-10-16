#include "CGameEntitySystem.h"

#include "Misc/Offsets.h"
#include "Misc/Patterns.h"
#include "SDK/EntitySystem/CEntityIdentity.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <cstdint>

namespace
{
    std::uintptr_t EntitySystemPtr = 0;
}

bool CGameEntitySystem::Init()
{
    std::uintptr_t Site = Memory::FindPattern("client.dll", Patterns::GameEntitySystem);
    if (!Site)
    {
        LogE("GameEntitySystem pattern not found");
        return false;
    }
    EntitySystemPtr = Memory::ResolveRelative(Site, 3, 7);
    Log("g_pGameEntitySystem %p -> %p", reinterpret_cast<void*>(EntitySystemPtr), static_cast<void*>(Get()));
    return true;
}

CGameEntitySystem* CGameEntitySystem::Get()
{
    return EntitySystemPtr ? Memory::Read<CGameEntitySystem*>(EntitySystemPtr) : nullptr;
}

CEntityIdentity* CGameEntitySystem::Identity(int Index) const
{
    if (Index < 0 || Index >= MaxEntities)
        return nullptr;

    std::uintptr_t Chunk = Memory::Read<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(this) + Offsets::CGameEntitySystem::IdentityChunks +
                                                        sizeof(std::uintptr_t) * (Index / Offsets::CGameEntitySystem::ChunkSize));
    if (!Chunk)
        return nullptr;

    auto* Result = reinterpret_cast<CEntityIdentity*>(Chunk + Offsets::CGameEntitySystem::IdentityStride * (Index % Offsets::CGameEntitySystem::ChunkSize));
    return Result->Handle().Index() == static_cast<std::uint32_t>(Index) ? Result : nullptr;
}

C_BaseEntity* CGameEntitySystem::Entity(int Index) const
{
    CEntityIdentity* Id = Identity(Index);
    return Id ? Id->Entity() : nullptr;
}

C_BaseEntity* CGameEntitySystem::Entity(CHandle Handle) const
{
    if (!Handle.IsValid())
        return nullptr;

    CEntityIdentity* Id = Identity(static_cast<int>(Handle.Index()));
    return Id && Id->Handle() == Handle ? Id->Entity() : nullptr;
}
