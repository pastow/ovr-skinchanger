#pragma once

#include <cstdint>
#include <optional>

class C_EconItemView;

namespace WearableSwap
{
    struct Replacement
    {
        std::uint32_t DefIndex = 0;
        std::uint8_t Style = 0;
        std::uint64_t ItemId = 0;
        std::uint32_t AccountId = 0;
    };

    std::optional<Replacement> Find(const C_EconItemView* View);
}
