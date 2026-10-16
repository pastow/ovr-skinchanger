#pragma once

#include <cstdint>

class KeyValues3
{
public:
    bool HoldsPointer() const;
    void* Pointer() const;
    void SetPointer(void* Value);
};
