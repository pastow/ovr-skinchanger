#pragma once

#include <cstdint>

class C_EconItemView;

class CDOTAPlayerInventory
{
public:
    static bool Init();

    std::uint32_t OwnerAccountId() const;
    C_EconItemView* FindItem(std::uint64_t ItemId);
    C_EconItemView* GetItemInLoadout(std::uint32_t HeroId, std::uint32_t Slot);
};
