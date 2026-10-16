#pragma once

#include <cstdint>
#include <map>

namespace UnusualStore
{
    using Listener = void (*)(std::uint64_t ItemId, int EffectId);

    int Get(std::uint64_t ItemId);
    void Set(std::uint64_t ItemId, int EffectId);
    const std::map<std::uint64_t, int>& All();

    void Subscribe(Listener Callback);
}
