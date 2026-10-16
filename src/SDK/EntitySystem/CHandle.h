#pragma once

#include <cstdint>

struct CHandle
{
    std::uint32_t Value = 0xFFFFFFFF;

    std::uint32_t Index() const { return Value & 0x7FFF; }
    bool IsValid() const { return Value != 0xFFFFFFFF && Index() != 0x7FFF; }
    bool operator==(const CHandle& Other) const { return Value == Other.Value; }
};
