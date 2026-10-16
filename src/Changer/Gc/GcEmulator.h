#pragma once

#include "base_gcmessages.pb.h"
#include "steammessages.pb.h"

#include <cstdint>
#include <vector>

namespace GcEmulator
{
    bool Parse(const void* Data, std::uint32_t Size, CMsgProtoBufHeader& Header, google::protobuf::MessageLite& Body);
    void Respond(std::uint32_t EMsg, const CMsgProtoBufHeader& Request, const google::protobuf::MessageLite& Body);
    void SendItems(const std::vector<CSOEconItem>& Added, const std::vector<CSOEconItem>& Modified);
}
