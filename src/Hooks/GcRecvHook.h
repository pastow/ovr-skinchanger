#pragma once

#include <cstdint>
#include <vector>

namespace GcRecvHook
{
    struct SoOwner
    {
        std::uint64_t Id = 0;
        std::uint64_t Version = 0;
        bool          Valid = false;
    };

    bool Install();
    void PushFake(std::vector<std::uint8_t> Message);
    void Pump();
    void Update();
    SoOwner Owner();
}
