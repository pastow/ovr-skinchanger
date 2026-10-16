#include "CUIPanel.h"

#include "Misc/Offsets.h"
#include "SDK/Memory/Memory.h"

#include <cstdint>

void* CUIPanel::Client() const
{
    using ClientFn = void* (*)(const CUIPanel*);
    auto Vtable = Memory::Read<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(this));
    auto Function = Memory::Read<ClientFn>(Vtable + Offsets::CUIPanel::ClientPtr);
    return Function(this);
}
