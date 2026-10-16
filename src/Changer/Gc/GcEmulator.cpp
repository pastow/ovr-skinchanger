#include "GcEmulator.h"

#include "Hooks/GcRecvHook.h"
#include "SDK/SteamAPI/ISteamUser.h"

#include "gcsdk_gcmessages.pb.h"

#include <cstring>
#include <string>

namespace
{
    constexpr std::uint32_t EMsgSoUpdateMultiple = 26;
    constexpr std::uint32_t ProtoFlag = 0x80000000u;
    constexpr int SoTypeEconItem = 1;
    constexpr std::uint32_t SoOwnerTypeUser = 1;

    void Push(std::uint32_t EMsg, const CMsgProtoBufHeader& Header, const std::string& Body)
    {
        std::string HeaderBytes = Header.SerializeAsString();
        std::vector<std::uint8_t> Out(8 + HeaderBytes.size() + Body.size());
        std::uint32_t Type = EMsg | ProtoFlag;
        std::uint32_t HeaderLen = static_cast<std::uint32_t>(HeaderBytes.size());
        std::memcpy(Out.data(), &Type, 4);
        std::memcpy(Out.data() + 4, &HeaderLen, 4);
        std::memcpy(Out.data() + 8, HeaderBytes.data(), HeaderBytes.size());
        std::memcpy(Out.data() + 8 + HeaderBytes.size(), Body.data(), Body.size());
        GcRecvHook::PushFake(std::move(Out));
        GcRecvHook::Pump();
    }
}

bool GcEmulator::Parse(const void* Data, std::uint32_t Size, CMsgProtoBufHeader& Header, google::protobuf::MessageLite& Body)
{
    auto* Bytes = static_cast<const std::uint8_t*>(Data);
    if (!Bytes || Size < 8)
        return false;
    std::uint32_t HeaderLen = 0;
    std::memcpy(&HeaderLen, Bytes + 4, 4);
    if (8 + HeaderLen > Size)
        return false;
    return Header.ParseFromArray(Bytes + 8, static_cast<int>(HeaderLen)) &&
           Body.ParseFromArray(Bytes + 8 + HeaderLen, static_cast<int>(Size - 8 - HeaderLen));
}

void GcEmulator::Respond(std::uint32_t EMsg, const CMsgProtoBufHeader& Request, const google::protobuf::MessageLite& Body)
{
    CMsgProtoBufHeader Header;
    Header.set_job_id_target(Request.job_id_source());
    Push(EMsg, Header, Body.SerializeAsString());
}

void GcEmulator::SendItems(const std::vector<CSOEconItem>& Added, const std::vector<CSOEconItem>& Modified)
{
    if (Added.empty() && Modified.empty())
        return;

    GcRecvHook::SoOwner Owner = GcRecvHook::Owner();
    if (!Owner.Valid)
    {
        ISteamUser* User = ISteamUser::Get();
        Owner.Id = User ? User->GetSteamID() : 0;
    }

    CMsgSOMultipleObjects Update;
    Update.mutable_owner_soid()->set_type(SoOwnerTypeUser);
    Update.mutable_owner_soid()->set_id(Owner.Id);
    Update.set_version(Owner.Version);
    for (const CSOEconItem& Item : Added)
    {
        auto* Object = Update.add_objects_added();
        Object->set_type_id(SoTypeEconItem);
        Object->set_object_data(Item.SerializeAsString());
    }
    for (const CSOEconItem& Item : Modified)
    {
        auto* Object = Update.add_objects_modified();
        Object->set_type_id(SoTypeEconItem);
        Object->set_object_data(Item.SerializeAsString());
    }
    Push(EMsgSoUpdateMultiple, CMsgProtoBufHeader(), Update.SerializeAsString());
}
