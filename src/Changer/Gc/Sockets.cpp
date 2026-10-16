#include "Sockets.h"

#include "Changer/Inventory/FakeItems.h"
#include "Changer/Gc/GcEmulator.h"
#include "Changer/Items/ItemSockets.h"
#include "Changer/Unusual/UnusualStore.h"
#include "SDK/Logger/Logger.h"

#include <vector>

namespace
{
    constexpr std::uint32_t EMsgAddItemToSocketResponse = 1089;
    constexpr std::uint32_t EMsgAddSocketResponse = 1090;
    constexpr std::uint32_t EMsgExtractGemsResponse = 1094;
    constexpr std::uint32_t AnySocket = 65535;

    void DropUnusual(std::uint64_t ItemId, std::uint32_t Socket)
    {
        auto Item = FakeItems::Find(ItemId);
        if (Item && Socket && ItemSockets::FindGem(*Item, ItemSockets::UnusualGemDef) == Socket)
            UnusualStore::Set(ItemId, 0);
    }
}

bool Sockets::OnAddSocket(const void* Data, std::uint32_t Size)
{
    CMsgProtoBufHeader Header;
    CMsgAddSocket Request;
    if (!GcEmulator::Parse(Data, Size, Header, Request) || !FakeItems::IsFake(Request.item_item_id()))
        return false;

    CMsgAddSocketResponse Response;
    Response.set_item_id(Request.item_item_id());

    auto Item = FakeItems::Find(Request.item_item_id());
    std::uint32_t Socket = Item ? ItemSockets::FreeSocket(*Item) : 0;
    if (Socket)
    {
        std::uint32_t RequiredType = ItemSockets::NextRequiredType(*Item);
        ItemSockets::Set(*Item, Socket, ItemSockets::EmptyBytes(RequiredType));
        FakeItems::Store(*Item);
        GcEmulator::SendItems({}, {*Item});
        Response.add_updated_socket_index(Socket);
        Response.set_response(CMsgAddSocketResponse::k_AddSocket_Succeeded);
    }
    else
    {
        Response.set_response(CMsgAddSocketResponse::k_AddSocket_Failed_FailedToAddSocket);
        LogW("Socket add failed %#llx", static_cast<unsigned long long>(Request.item_item_id()));
    }
    GcEmulator::Respond(EMsgAddSocketResponse, Header, Response);
    return true;
}

bool Sockets::OnAddGem(const void* Data, std::uint32_t Size)
{
    CMsgProtoBufHeader Header;
    CMsgAddItemToSocket Request;
    if (!GcEmulator::Parse(Data, Size, Header, Request) || !FakeItems::IsFake(Request.item_item_id()))
        return false;

    std::uint64_t ItemId = Request.item_item_id();
    CMsgAddItemToSocketResponse Response;
    Response.set_item_item_id(ItemId);
    bool Ok = Request.gems_to_socket_size() > 0;

    for (const auto& Entry : Request.gems_to_socket())
    {
        if (ItemSockets::IsSocket(Entry.socket_index()))
            DropUnusual(ItemId, Entry.socket_index());

        auto Item = FakeItems::Find(ItemId);
        auto Gem = FakeItems::Find(Entry.gem_item_id());
        auto Value = Gem ? ItemSockets::Bytes(*Gem, ItemSockets::GemValueSocket) : std::nullopt;
        if (!Item || !Value)
        {
            LogW("Gem add failed item %#llx gem %#llx", static_cast<unsigned long long>(ItemId),
                 static_cast<unsigned long long>(Entry.gem_item_id()));
            Ok = false;
            continue;
        }

        std::uint32_t Socket = ItemSockets::IsSocket(Entry.socket_index()) ? Entry.socket_index() : ItemSockets::FirstEmptySocket(*Item);
        if (!Socket)
        {
            Ok = false;
            continue;
        }
        ItemSockets::Set(*Item, Socket, *Value);
        FakeItems::Store(*Item);
        GcEmulator::SendItems({}, {*Item});
        Response.add_updated_socket_index(Socket);
    }

    Response.set_response(Ok ? CMsgAddItemToSocketResponse::k_AddGem_Succeeded : CMsgAddItemToSocketResponse::k_AddGem_Failed_FailedToAddGem);
    GcEmulator::Respond(EMsgAddItemToSocketResponse, Header, Response);
    return true;
}

bool Sockets::OnExtractGems(const void* Data, std::uint32_t Size)
{
    CMsgProtoBufHeader Header;
    CMsgExtractGems Request;
    if (!GcEmulator::Parse(Data, Size, Header, Request) || !FakeItems::IsFake(Request.item_item_id()))
        return false;

    std::uint64_t ItemId = Request.item_item_id();
    std::vector<std::uint32_t> Targets;
    if (auto Item = FakeItems::Find(ItemId))
    {
        for (std::uint32_t Socket = ItemSockets::FirstSocket; Socket <= ItemSockets::LastSocket; ++Socket)
            if (Request.item_socket_id() == AnySocket || Request.item_socket_id() == Socket)
                Targets.push_back(Socket);
    }

    for (std::uint32_t Socket : Targets)
        DropUnusual(ItemId, Socket);

    CMsgExtractGemsResponse Response;
    Response.set_item_id(ItemId);
    auto Item = FakeItems::Find(ItemId);
    if (Item)
    {
        for (std::uint32_t Socket : Targets)
        {
            auto Value = ItemSockets::Bytes(*Item, Socket);
            auto Current = Value ? ItemSockets::Read(*Value) : std::nullopt;
            if (Current && Current->gem_def_index())
                ItemSockets::Set(*Item, Socket, ItemSockets::EmptyBytes(Current->required_type()));
        }
        FakeItems::Store(*Item);
        GcEmulator::SendItems({}, {*Item});
        Response.set_response(CMsgExtractGemsResponse::k_ExtractGems_Succeeded);
    }
    else
    {
        Response.set_response(CMsgExtractGemsResponse::k_ExtractGems_Failed_ItemIsInvalid);
    }
    GcEmulator::Respond(EMsgExtractGemsResponse, Header, Response);
    return true;
}
