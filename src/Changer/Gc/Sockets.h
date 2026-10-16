#pragma once

#include <cstdint>

namespace Sockets
{
    bool OnAddSocket(const void* Data, std::uint32_t Size);
    bool OnAddGem(const void* Data, std::uint32_t Size);
    bool OnExtractGems(const void* Data, std::uint32_t Size);
}
