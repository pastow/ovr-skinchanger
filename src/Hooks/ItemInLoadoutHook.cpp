#include "ItemInLoadoutHook.h"

#include "Changer/Match/LoadoutOverride.h"
#include "Misc/Patterns.h"
#include "SDK/Econ/CDOTAPlayerInventory.h"
#include "SDK/Econ/C_EconItemView.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <MinHook.h>

#include <cstdint>

namespace
{
    C_EconItemView* (*Original)(CDOTAPlayerInventory*, std::uint32_t, std::uint32_t, bool) = nullptr;

    C_EconItemView* Detour(CDOTAPlayerInventory* Inventory, std::uint32_t HeroId, std::uint32_t Slot, bool Flag)
    {
        return LoadoutOverride::Apply(Inventory, HeroId, Slot, Original(Inventory, HeroId, Slot, Flag));
    }
}

bool ItemInLoadoutHook::Install()
{
    std::uintptr_t Target = Memory::FindPattern("client.dll", Patterns::GetItemInLoadout);
    if (!Target)
    {
        LogE("GetItemInLoadout pattern not found");
        return false;
    }

    MH_STATUS Status = MH_Initialize();
    if (Status != MH_OK && Status != MH_ERROR_ALREADY_INITIALIZED)
    {
        LogE("MinHook init failed: %s", MH_StatusToString(Status));
        return false;
    }

    auto* TargetFn = reinterpret_cast<void*>(Target);
    Status = MH_CreateHook(TargetFn, reinterpret_cast<void*>(Detour), reinterpret_cast<void**>(&Original));
    if (Status == MH_OK)
        Status = MH_EnableHook(TargetFn);

    if (Status != MH_OK)
    {
        LogE("GetItemInLoadout hook failed: %s", MH_StatusToString(Status));
        return false;
    }

    Log("GetItemInLoadout %#llx", static_cast<unsigned long long>(Target));
    return true;
}
