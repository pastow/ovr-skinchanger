#include "C_DOTA_BaseNPC_Hero.h"

#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"
#include "SDK/SchemaSystem/SchemaOffset.h"

namespace
{
    std::uint32_t PlayerIDOffset = 0;
    std::uint32_t HeroIdOffset = 0;
    std::uint32_t ReplicatingOtherHeroModelOffset = 0;
}

bool C_DOTA_BaseNPC_Hero::Init()
{
    PlayerIDOffset = SchemaOffset::Instance()->GetOffset("C_DOTA_BaseNPC_Hero", "m_iPlayerID");
    HeroIdOffset = SchemaOffset::Instance()->GetOffset("C_DOTA_BaseNPC_Hero", "m_iHeroID");
    if (!HeroIdOffset)
        LogW("C_DOTA_BaseNPC_Hero::m_iHeroID not found in schema");
    ReplicatingOtherHeroModelOffset = SchemaOffset::Instance()->GetOffset("C_DOTA_BaseNPC_Hero", "m_hReplicatingOtherHeroModel");
    if (!PlayerIDOffset || !ReplicatingOtherHeroModelOffset)
    {
        LogE("C_DOTA_BaseNPC_Hero fields not found in schema m_iPlayerID 0x%X m_hReplicatingOtherHeroModel 0x%X", PlayerIDOffset, ReplicatingOtherHeroModelOffset);
        return false;
    }
    return true;
}

std::int32_t C_DOTA_BaseNPC_Hero::PlayerID() const
{
    return Memory::Read<std::int32_t>(reinterpret_cast<std::uintptr_t>(this) + PlayerIDOffset);
}

CHandle C_DOTA_BaseNPC_Hero::ReplicatingOtherHeroModel() const
{
    return CHandle {Memory::Read<std::uint32_t>(reinterpret_cast<std::uintptr_t>(this) + ReplicatingOtherHeroModelOffset)};
}

bool C_DOTA_BaseNPC_Hero::IsIllusion() const
{
    return ReplicatingOtherHeroModel().IsValid();
}

std::uint32_t C_DOTA_BaseNPC_Hero::HeroId() const
{
    return HeroIdOffset ? Memory::Read<std::uint32_t>(reinterpret_cast<std::uintptr_t>(this) + HeroIdOffset) : 0;
}
