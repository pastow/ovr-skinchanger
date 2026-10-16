#pragma once

#include "base_gcmessages.pb.h"

#include <cstdint>

namespace ItemStyles
{
    void UnlockAll(CSOEconItem& Item);
    bool OnSetStyle(const void* Data, std::uint32_t Size);
}
