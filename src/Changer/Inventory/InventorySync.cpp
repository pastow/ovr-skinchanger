#include "InventorySync.h"

#include "Changer/Inventory/FakeItems.h"
#include "Changer/Gc/GcEmulator.h"
#include "Changer/Inventory/GemCatalog.h"
#include "Changer/Items/ItemSockets.h"
#include "Changer/Gc/ItemStyles.h"
#include "Changer/Inventory/RealItems.h"
#include "Changer/Unusual/UnusualStore.h"
#include "SDK/GC/CGCClient.h"
#include "SDK/Logger/Logger.h"
#include "SDK/SteamAPI/ISteamUser.h"

#include "gcsdk_gcmessages.pb.h"

#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr int SoTypeEconItem = 1;
    constexpr std::uint32_t SoOwnerTypeUser = 1;
    constexpr std::uint32_t EMsgSoCreate = 21;
    constexpr std::uint32_t EMsgSoUpdate = 22;
    constexpr std::uint32_t EMsgSoDestroy = 23;
    constexpr std::uint32_t EMsgSoCacheSubscribed = 24;
    constexpr std::uint32_t EMsgSoUpdateMultiple = 26;

    std::mutex ShownMutex;
    std::unordered_map<std::uint64_t, std::string> Shown;
    bool Started = false;
    bool Subscribed = false;

    bool IsLocalOwner(const CMsgSOIDOwner& Owner)
    {
        ISteamUser* User = ISteamUser::Get();
        return Owner.type() == SoOwnerTypeUser && User && Owner.id() == User->GetSteamID();
    }

    std::set<std::uint32_t> OccupiedSlots()
    {
        std::set<std::uint32_t> Occupied;
        FakeItems::ForEach([&](CSOEconItem& Item) {
            for (const auto& State : Item.equipped_state())
                Occupied.insert((State.new_slot() << 16) | (State.new_class() & 0xFFFF));
        });
        return Occupied;
    }

    CSOEconItem Desired(const CSOEconItem& Pristine, const std::set<std::uint32_t>& Occupied)
    {
        CSOEconItem Out = Pristine;
        auto* States = Out.mutable_equipped_state();
        for (int Index = States->size() - 1; Index >= 0; --Index)
        {
            const auto& State = States->Get(Index);
            if (Occupied.count((State.new_slot() << 16) | (State.new_class() & 0xFFFF)))
                States->DeleteSubrange(Index, 1);
        }
        if (int Effect = UnusualStore::Get(Pristine.id()))
            ItemSockets::SetUnusual(Out, Effect, Pristine.quality());
        return Out;
    }

    bool ReplaceReal(std::string& Data, const std::set<std::uint32_t>& Occupied)
    {
        CSOEconItem Item;
        if (!Item.ParseFromString(Data) || !Item.id() || FakeItems::IsFake(Item.id()))
            return false;
        RealItems::Set(Item);
        std::string Out = Desired(Item, Occupied).SerializeAsString();
        {
            std::lock_guard<std::mutex> Lock(ShownMutex);
            Shown[Item.id()] = Out;
        }
        if (Out == Data)
            return false;
        Data = std::move(Out);
        return true;
    }

    void Forget(const std::string& Data)
    {
        CSOEconItem Item;
        if (!Item.ParseFromString(Data) || !Item.id())
            return;
        RealItems::Remove(Item.id());
        std::lock_guard<std::mutex> Lock(ShownMutex);
        Shown.erase(Item.id());
    }

    std::vector<CSOEconItem> PreparedFakes()
    {
        GemCatalog::EnsureGranted();
        std::vector<CSOEconItem> Items = FakeItems::All();
        for (CSOEconItem& Item : Items)
        {
            ItemStyles::UnlockAll(Item);
            int Effect = UnusualStore::Get(Item.id());
            if (Effect || ItemSockets::FindGem(Item, ItemSockets::UnusualGemDef))
                ItemSockets::SetUnusual(Item, Effect, ItemSockets::QualityUnique);
        }
        FakeItems::Store(Items);
        return Items;
    }

    void OnUnusual(std::uint64_t ItemId, int Effect)
    {
        if (!FakeItems::IsFake(ItemId))
        {
            InventorySync::Refresh();
            return;
        }
        auto Item = FakeItems::Find(ItemId);
        if (!Item)
            return;
        ItemSockets::SetUnusual(*Item, Effect, ItemSockets::QualityUnique);
        FakeItems::Store(*Item);
        GcEmulator::SendItems({}, {*Item});
    }

    void EnsureSubscribed()
    {
        if (Subscribed)
            return;
        Subscribed = true;
        UnusualStore::Subscribe(OnUnusual);
    }

    bool RewriteSubscribed(std::string& Body)
    {
        CMsgSOCacheSubscribed Message;
        if (!Message.ParseFromString(Body) || !IsLocalOwner(Message.owner_soid()))
            return false;

        int Existing = 0;
        for (const auto& Type : Message.objects())
            if (Type.type_id() == SoTypeEconItem)
                Existing += Type.object_data_size();
        if (Existing == 0)
            return false;

        EnsureSubscribed();
        Started = true;
        RealItems::Clear();
        {
            std::lock_guard<std::mutex> Lock(ShownMutex);
            Shown.clear();
        }

        std::set<std::uint32_t> Occupied = OccupiedSlots();
        CMsgSOCacheSubscribed::SubscribedType* Items = nullptr;
        for (auto& Type : *Message.mutable_objects())
        {
            if (Type.type_id() != SoTypeEconItem)
                continue;
            Items = &Type;
            for (std::string& Data : *Type.mutable_object_data())
                ReplaceReal(Data, Occupied);
        }
        if (!Items)
        {
            Items = Message.add_objects();
            Items->set_type_id(SoTypeEconItem);
        }

        std::vector<CSOEconItem> Fakes = PreparedFakes();
        for (const CSOEconItem& Fake : Fakes)
            Items->add_object_data(Fake.SerializeAsString());

        Body = Message.SerializeAsString();
        return true;
    }

    bool RewriteMultiple(std::string& Body)
    {
        CMsgSOMultipleObjects Message;
        if (!Message.ParseFromString(Body) || !IsLocalOwner(Message.owner_soid()))
            return false;

        std::set<std::uint32_t> Occupied = OccupiedSlots();
        bool Changed = false;
        for (auto& Object : *Message.mutable_objects_added())
            if (Object.type_id() == SoTypeEconItem)
                Changed |= ReplaceReal(*Object.mutable_object_data(), Occupied);
        for (auto& Object : *Message.mutable_objects_modified())
            if (Object.type_id() == SoTypeEconItem)
                Changed |= ReplaceReal(*Object.mutable_object_data(), Occupied);
        for (const auto& Object : Message.objects_removed())
            if (Object.type_id() == SoTypeEconItem)
                Forget(Object.object_data());
        if (!Changed)
            return false;
        Body = Message.SerializeAsString();
        return true;
    }

    bool RewriteSingle(std::uint32_t EMsg, std::string& Body)
    {
        CMsgSOSingleObject Message;
        if (!Message.ParseFromString(Body) || !IsLocalOwner(Message.owner_soid()) || Message.type_id() != SoTypeEconItem)
            return false;
        if (EMsg == EMsgSoDestroy)
        {
            Forget(Message.object_data());
            return false;
        }
        if (!ReplaceReal(*Message.mutable_object_data(), OccupiedSlots()))
            return false;
        Body = Message.SerializeAsString();
        return true;
    }
}

void InventorySync::Update()
{
    if (Started || !CGCClient::Get() || !ISteamUser::Get())
        return;
    Started = true;
    EnsureSubscribed();
    std::vector<CSOEconItem> Fakes = PreparedFakes();
    GcEmulator::SendItems(Fakes, {});
    Refresh();
}

void InventorySync::Refresh()
{
    std::set<std::uint32_t> Occupied = OccupiedSlots();
    std::vector<CSOEconItem> Modified;
    RealItems::ForEach([&](const CSOEconItem& Pristine) {
        CSOEconItem Item = Desired(Pristine, Occupied);
        std::string Data = Item.SerializeAsString();
        std::lock_guard<std::mutex> Lock(ShownMutex);
        std::string& Current = Shown[Pristine.id()];
        if (Current == Data)
            return;
        Current = Data;
        Modified.push_back(Item);
    });

    if (Modified.empty())
        return;
    GcEmulator::SendItems({}, Modified);
}

bool InventorySync::Rewrite(std::uint32_t EMsg, std::string& Body)
{
    switch (EMsg)
    {
    case EMsgSoCacheSubscribed:
        return RewriteSubscribed(Body);
    case EMsgSoUpdateMultiple:
        return RewriteMultiple(Body);
    case EMsgSoCreate:
    case EMsgSoUpdate:
    case EMsgSoDestroy:
        return RewriteSingle(EMsg, Body);
    default:
        return false;
    }
}
