#include "CDOTAPlayerInventory.h"

#include "Misc/Offsets.h"
#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

namespace
{
    constexpr std::uint32_t OwnerTypeUser = 1;
    constexpr std::uint32_t OwnerTypeShift = 16;

    C_EconItemView* (*FindItemFn)(CDOTAPlayerInventory*, std::uint64_t, int*) = nullptr;
    C_EconItemView* (*GetItemInLoadoutFn)(CDOTAPlayerInventory*, std::uint32_t, std::uint32_t, bool) = nullptr;
}

bool CDOTAPlayerInventory::Init()
{
    FindItemFn = reinterpret_cast<decltype(FindItemFn)>(Memory::FindPattern("client.dll", Patterns::InventoryFindItem));
    GetItemInLoadoutFn = reinterpret_cast<decltype(GetItemInLoadoutFn)>(Memory::FindPattern("client.dll", Patterns::GetItemInLoadout));
    if (!FindItemFn || !GetItemInLoadoutFn)
    {
        LogE("Inventory patterns not found FindItem %p GetItemInLoadout %p", reinterpret_cast<void*>(FindItemFn),
             reinterpret_cast<void*>(GetItemInLoadoutFn));
        return false;
    }
    return true;
}

std::uint32_t CDOTAPlayerInventory::OwnerAccountId() const
{
    std::uintptr_t Self = reinterpret_cast<std::uintptr_t>(this);
    std::uint32_t Type = Memory::Read<std::uint32_t>(Self + Offsets::CDOTAPlayerInventory::OwnerType) >> OwnerTypeShift;
    return Type == OwnerTypeUser ? Memory::Read<std::uint32_t>(Self + Offsets::CDOTAPlayerInventory::OwnerId) : 0;
}

C_EconItemView* CDOTAPlayerInventory::FindItem(std::uint64_t ItemId)
{
    return FindItemFn(this, ItemId, nullptr);
}

C_EconItemView* CDOTAPlayerInventory::GetItemInLoadout(std::uint32_t HeroId, std::uint32_t Slot)
{
    return GetItemInLoadoutFn(this, HeroId, Slot, false);
}
