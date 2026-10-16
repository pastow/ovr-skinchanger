#include "LoadoutOverride.h"

#include "Changer/Inventory/FakeItems.h"
#include "SDK/Econ/CDOTAInventoryManager.h"
#include "SDK/Econ/CDOTAPlayerInventory.h"
#include "SDK/Econ/C_EconItemView.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTAPlayerController.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTA_BaseNPC_Hero.h"
#include "SDK/Logger/Logger.h"
#include "SDK/SteamAPI/ISteamUser.h"

#include <mutex>
#include <set>
#include <tuple>

namespace
{
    std::mutex LoggedMutex;
    std::set<std::tuple<std::uint32_t, std::uint32_t, std::uint64_t, std::uint64_t>> Logged;

    std::uint64_t EquippedFake(std::uint32_t HeroId, std::uint32_t Slot)
    {
        std::uint64_t Found = 0;
        FakeItems::ForEach([&](CSOEconItem& Item) {
            for (const auto& State : Item.equipped_state())
                if (State.new_class() == HeroId && State.new_slot() == Slot)
                    Found = Item.id();
        });
        return Found;
    }

    void LogOnce(std::uint32_t HeroId, std::uint32_t Slot, std::uint64_t From, std::uint64_t To, bool Found)
    {
        std::lock_guard<std::mutex> Lock(LoggedMutex);
        if (!Logged.insert({HeroId, Slot, From, To}).second)
            return;
        Log("Loadout hero %u slot %u %#llx -> %#llx %s", HeroId, Slot, static_cast<unsigned long long>(From), static_cast<unsigned long long>(To),
            Found ? "fake" : "fake not in local inventory");
    }
}

C_EconItemView* LoadoutOverride::Apply(CDOTAPlayerInventory* Inventory, std::uint32_t HeroId, std::uint32_t Slot, C_EconItemView* Current)
{
    ISteamUser* User = ISteamUser::Get();
    if (!Inventory || !User || Inventory->OwnerAccountId() != User->AccountId())
        return Current;

    C_DOTA_BaseNPC_Hero* LocalHero = C_DOTAPlayerController::LocalHero();
    std::uint32_t LocalHeroId = LocalHero ? LocalHero->HeroId() : 0;
    if (LocalHeroId && HeroId != LocalHeroId)
        return Current;

    std::uint64_t FakeId = EquippedFake(HeroId, Slot);
    if (!FakeId)
        return Current;
    std::uint64_t CurrentId = Current ? Current->ItemId() : 0;
    if (CurrentId == FakeId)
        return Current;

    CDOTAInventoryManager* Manager = CDOTAInventoryManager::Get();
    CDOTAPlayerInventory* Local = Manager ? Manager->LocalInventory() : nullptr;
    C_EconItemView* Fake = Local ? Local->FindItem(FakeId) : nullptr;
    if (!Fake && Local != Inventory)
        Fake = Inventory->FindItem(FakeId);
    LogOnce(HeroId, Slot, CurrentId, FakeId, Fake != nullptr);
    return Fake ? Fake : Current;
}
