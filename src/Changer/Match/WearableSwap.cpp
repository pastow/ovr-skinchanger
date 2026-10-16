#include "WearableSwap.h"

#include "Changer/Inventory/FakeItems.h"
#include "Changer/Match/NetSkins.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTAPlayerController.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTA_BaseNPC_Hero.h"
#include "SDK/Econ/CDOTAInventoryManager.h"
#include "SDK/Econ/CDOTAPlayerInventory.h"
#include "SDK/Econ/CEconItemDefinition.h"
#include "SDK/Econ/CEconItemSchema.h"
#include "SDK/Econ/C_EconItemView.h"
#include "SDK/Logger/Logger.h"
#include "SDK/SteamAPI/ISteamUser.h"

namespace
{
    constexpr std::uint32_t StyleUnset = 0xFF;
    constexpr std::uint32_t LoadoutSlots = 32;
}

std::optional<WearableSwap::Replacement> WearableSwap::Find(const C_EconItemView* View)
{
    CEconItemSchema* Schema = CEconItemSchema::Get();
    ISteamUser* User = ISteamUser::Get();
    if (!Schema || !User)
        return std::nullopt;

    std::uint32_t Def = View->ItemDefinitionIndex();
    std::uint32_t Account = View->AccountId();
    std::uint32_t LocalAccount = User->AccountId();
    CEconItemDefinition* RealDef = Schema->GetItemDefinition(Def);
    if (!RealDef)
        return std::nullopt;

    std::uint32_t Hero = RealDef->HeroId();
    std::uint32_t Slot = RealDef->LoadoutSlot();
    C_DOTA_BaseNPC_Hero* LocalHero = C_DOTAPlayerController::LocalHero();
    std::uint32_t LocalHeroId = LocalHero ? LocalHero->HeroId() : 0;
    if (LocalHeroId && Hero != LocalHeroId)
        return std::nullopt;
    if (Account != 0 && Account != LocalAccount)
        return std::nullopt;

    std::optional<Replacement> Result;
    CDOTAInventoryManager* Manager = CDOTAInventoryManager::Get();
    CDOTAPlayerInventory* Inventory = Manager ? Manager->LocalInventory() : nullptr;
    for (std::uint32_t Index = 0; Inventory && !Result && Index < LoadoutSlots; ++Index)
    {
        C_EconItemView* Equipped = Inventory->GetItemInLoadout(Hero, Index);
        if (!Equipped || !FakeItems::IsFake(Equipped->ItemId()))
            continue;
        CEconItemDefinition* FakeDef = Schema->GetItemDefinition(Equipped->ItemDefinitionIndex());
        if (!FakeDef || FakeDef->LoadoutSlot() != Slot)
            continue;
        auto Item = FakeItems::Find(Equipped->ItemId());
        if (!Item)
            continue;
        Replacement Out;
        Out.DefIndex = Item->def_index();
        Out.Style = Item->style() == StyleUnset ? 0 : static_cast<std::uint8_t>(Item->style());
        Out.ItemId = Item->id();
        Out.AccountId = LocalAccount;
        Result = Out;
    }

    return Result;
}
