#include "KeyValues3.h"

#include "Misc/Offsets.h"
#include "SDK/Memory/Memory.h"

namespace
{
    constexpr std::uint64_t TypeShift = 2;
    constexpr std::uint64_t TypeMask = 0xF;
    constexpr std::uint64_t SubTypeShift = 10;
    constexpr std::uint64_t SubTypeMask = 0xFF;
    constexpr std::uint64_t TypePointer = 3;
    constexpr std::uint64_t TypeTyped = 4;
    constexpr std::uint64_t SubTypeString = 27;
}

bool KeyValues3::HoldsPointer() const
{
    std::uint64_t Header = Memory::Read<std::uint64_t>(reinterpret_cast<std::uintptr_t>(this));
    std::uint64_t Type = (Header >> TypeShift) & TypeMask;
    if (Type == TypePointer)
        return true;
    return Type == TypeTyped && ((Header >> SubTypeShift) & SubTypeMask) != SubTypeString;
}

void* KeyValues3::Pointer() const
{
    return Memory::Read<void*>(reinterpret_cast<std::uintptr_t>(this) + Offsets::KeyValues3::Data);
}

void KeyValues3::SetPointer(void* Value)
{
    *reinterpret_cast<void**>(reinterpret_cast<std::uintptr_t>(this) + Offsets::KeyValues3::Data) = Value;
}
