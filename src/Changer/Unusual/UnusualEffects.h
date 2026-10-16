#pragma once

#include <span>

namespace UnusualEffects
{
    struct Effect
    {
        const char* Name;
        int Id;
    };

    std::span<const Effect> All();
    const Effect* Find(int Id);
}
