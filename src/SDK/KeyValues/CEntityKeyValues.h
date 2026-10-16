#pragma once

#include <cstdint>

class KeyValues3;

class CEntityKeyValues
{
public:
    static bool Init();

    KeyValues3* FindValue(std::uint32_t Hash, const char* Name);
};
