#pragma once

#include <cstdint>
#include <vector>

namespace ItemGrant
{
    bool Grant(std::uint32_t Def);
    bool Grant(const std::vector<std::uint32_t>& Defs);
}
