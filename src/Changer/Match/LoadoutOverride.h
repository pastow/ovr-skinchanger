#pragma once

#include <cstdint>

class C_EconItemView;
class CDOTAPlayerInventory;

namespace LoadoutOverride
{
    C_EconItemView* Apply(CDOTAPlayerInventory* Inventory, std::uint32_t HeroId, std::uint32_t Slot, C_EconItemView* Current);
}
