#pragma once

#include <cstdint>

class ISteamUser
{
public:
    static bool Init();
    static ISteamUser* Get();

    std::uint64_t GetSteamID();
    std::uint32_t AccountId();
};
