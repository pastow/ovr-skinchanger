#pragma once

#include <cstdint>

namespace NetSkins
{
    void Update();
    int LocalHeroIndex();
    bool IsSwapped(int Index, std::uint64_t ModelId);
    bool IsOwnHero(int Index);
    std::uint64_t DefaultModelOf(int Index);
}
