#pragma once

#include "base_gcmessages.pb.h"

#include <cstdint>
#include <functional>

namespace RealItems
{
    void Clear();
    void Set(const CSOEconItem& Item);
    void Remove(std::uint64_t ItemId);
    void ForEach(const std::function<void(const CSOEconItem&)>& Visit);
}
