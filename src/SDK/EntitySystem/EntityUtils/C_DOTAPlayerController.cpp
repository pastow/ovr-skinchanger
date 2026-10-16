#include "C_DOTAPlayerController.h"

#include "Misc/Patterns.h"
#include "SDK/EntitySystem/CGameEntitySystem.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTA_BaseNPC_Hero.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"
#include "SDK/SchemaSystem/SchemaOffset.h"

#include <cstdint>

namespace
{
    std::uintptr_t LocalControllers = 0;
    std::uint32_t AssignedHeroOffset = 0;
}

bool C_DOTAPlayerController::Init()
{
    std::uintptr_t Site = Memory::FindPattern("client.dll", Patterns::LocalPlayerController);
    if (!Site)
    {
        LogE("LocalPlayerController pattern not found");
        return false;
    }
    LocalControllers = Memory::ResolveRelative(Site, 14, 18);

    AssignedHeroOffset = SchemaOffset::Instance()->GetOffset("C_DOTAPlayerController", "m_hAssignedHero");
    if (!AssignedHeroOffset)
    {
        LogE("C_DOTAPlayerController::m_hAssignedHero not found in schema");
        return false;
    }

    Log("LocalPlayerControllers %p m_hAssignedHero 0x%X", reinterpret_cast<void*>(LocalControllers), AssignedHeroOffset);
    return true;
}

C_DOTAPlayerController* C_DOTAPlayerController::GetLocal()
{
    return LocalControllers ? Memory::Read<C_DOTAPlayerController*>(LocalControllers) : nullptr;
}

C_DOTA_BaseNPC_Hero* C_DOTAPlayerController::LocalHero()
{
    C_DOTAPlayerController* Controller = GetLocal();
    CGameEntitySystem* System = CGameEntitySystem::Get();
    if (!Controller || !System)
        return nullptr;
    return System->Entity<C_DOTA_BaseNPC_Hero>(Controller->AssignedHero());
}

CHandle C_DOTAPlayerController::AssignedHero() const
{
    return CHandle {Memory::Read<std::uint32_t>(reinterpret_cast<std::uintptr_t>(this) + AssignedHeroOffset)};
}
