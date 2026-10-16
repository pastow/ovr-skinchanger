#include "CDOTA_DB_Page_OwnedItemDetails.h"

#include "Misc/Offsets.h"
#include "SDK/Memory/Memory.h"

#include <cstdint>
#include <cstring>

namespace
{
    constexpr const char* TypeName = ".?AVCDOTA_DB_Page_OwnedItemDetails@@";
}

CDOTA_DB_Page_OwnedItemDetails* CDOTA_DB_Page_OwnedItemDetails::FromPanel(const Panel& Page)
{
    CUIPanel* Native = Page.Native();
    if (!Native)
        return nullptr;

    void* Client = Native->Client();
    const char* Name = Memory::RttiName(Client);
    if (!Name || std::strcmp(Name, TypeName) != 0)
        return nullptr;
    return static_cast<CDOTA_DB_Page_OwnedItemDetails*>(Client);
}

C_EconItemView* CDOTA_DB_Page_OwnedItemDetails::Item() const
{
    return reinterpret_cast<C_EconItemView*>(reinterpret_cast<std::uintptr_t>(this) + Offsets::CDOTA_DB_Page_OwnedItemDetails::Item);
}
