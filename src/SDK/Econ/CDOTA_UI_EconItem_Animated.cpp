#include "CDOTA_UI_EconItem_Animated.h"

#include "Misc/Offsets.h"
#include "SDK/Memory/Memory.h"

#include <cstdint>
#include <cstring>

namespace
{
    constexpr const char* TypeName = ".?AVCDOTA_UI_EconItem_Animated@@";
}

CDOTA_UI_EconItem_Animated* CDOTA_UI_EconItem_Animated::FromNative(CUIPanel* Native)
{
    if (!Native)
        return nullptr;

    void* Client = Native->Client();
    const char* Name = Memory::RttiName(Client);
    if (!Name || std::strcmp(Name, TypeName) != 0)
        return nullptr;
    return static_cast<CDOTA_UI_EconItem_Animated*>(Client);
}

C_EconItemView* CDOTA_UI_EconItem_Animated::Item() const
{
    return reinterpret_cast<C_EconItemView*>(reinterpret_cast<std::uintptr_t>(this) + Offsets::CDOTA_UI_EconItem_Animated::Item);
}

CEconItemDefinition* CDOTA_UI_EconItem_Animated::ItemDefinition() const
{
    return Memory::Read<CEconItemDefinition*>(reinterpret_cast<std::uintptr_t>(this) + Offsets::CDOTA_UI_EconItem_Animated::ItemDefinition);
}
