#include "ItemStyles.h"

#include "Changer/Inventory/FakeItems.h"
#include "Changer/Gc/GcEmulator.h"
#include "Changer/Items/ItemSockets.h"
#include "SDK/Logger/Logger.h"

#include "econ_gcmessages.pb.h"

#include <string>

namespace
{
    constexpr std::uint32_t UnlockedStylesAttribute = 400;
    constexpr std::uint32_t EMsgSetItemStyleResponse = 2578;
    constexpr std::uint32_t StyleUnchanged = 255;
    const std::string AllStylesMask("\xFF\xFF\xFF\xFF", 4);
}

void ItemStyles::UnlockAll(CSOEconItem& Item)
{
    ItemSockets::Set(Item, UnlockedStylesAttribute, AllStylesMask);
}

bool ItemStyles::OnSetStyle(const void* Data, std::uint32_t Size)
{
    CMsgProtoBufHeader Header;
    CMsgClientToGCSetItemStyle Request;
    if (!GcEmulator::Parse(Data, Size, Header, Request) || !FakeItems::IsFake(Request.item_id()))
        return false;

    CMsgClientToGCSetItemStyleResponse Response;
    auto Item = FakeItems::Find(Request.item_id());
    if (Item && Request.style_index() != StyleUnchanged)
    {
        Item->set_style(Request.style_index());
        UnlockAll(*Item);
        FakeItems::Store(*Item);
        GcEmulator::SendItems({}, {*Item});
        Response.set_response(CMsgClientToGCSetItemStyleResponse::k_SetStyle_Succeeded);
    }
    else
    {
        Response.set_response(CMsgClientToGCSetItemStyleResponse::k_SetStyle_Failed);
        LogW("Style set failed %#llx style %u", static_cast<unsigned long long>(Request.item_id()), Request.style_index());
    }
    GcEmulator::Respond(EMsgSetItemStyleResponse, Header, Response);
    return true;
}
