#pragma once

#include <cstdint>

class CEconItemDefinition;

class CEconItemSchema
{
public:
    static bool Init();
    static CEconItemSchema* Get();

    CEconItemDefinition* GetItemDefinition(std::uint32_t DefIndex);
};
