#include "CNetMessage.h"

#include "Misc/Offsets.h"
#include "SDK/Memory/Memory.h"

namespace
{
    template <typename Fn>
    Fn Virtual(const void* Object, std::size_t Index)
    {
        return reinterpret_cast<Fn>(Memory::Read<void**>(reinterpret_cast<std::uintptr_t>(Object))[Index]);
    }
}

std::uint32_t CNetMessage::Id() const
{
    return Virtual<std::uint32_t (*)(const CNetMessage*)>(this, Offsets::CNetMessage::IdIndex)(this);
}

CProtobufMessage* CNetMessage::Proto() const
{
    return Virtual<CProtobufMessage* (*)(const CNetMessage*)>(this, Offsets::CNetMessage::ProtoIndex)(this);
}
