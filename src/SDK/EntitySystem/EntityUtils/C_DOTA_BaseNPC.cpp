#include "C_DOTA_BaseNPC.h"

#include "Misc/Offsets.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"
#include "SDK/SchemaSystem/SchemaOffset.h"

#include <cstdint>

namespace
{
    std::uint32_t MyWearablesOffset = 0;
    std::uint32_t NetworkActivityOffset = 0;
    std::uint32_t NetworkSequenceIndexOffset = 0;
    std::uint32_t ActivityModifiersOffset = 0;
}

bool C_DOTA_BaseNPC::Init()
{
    MyWearablesOffset = SchemaOffset::Instance()->GetOffset("C_BaseCombatCharacter", "m_hMyWearables");
    SchemaOffset* Schema = SchemaOffset::Instance();
    NetworkActivityOffset = Schema->GetOffset("C_DOTA_BaseNPC", "m_NetworkActivity");
    NetworkSequenceIndexOffset = Schema->GetOffset("C_DOTA_BaseNPC", "m_NetworkSequenceIndex");
    ActivityModifiersOffset = Schema->GetOffset("C_DOTA_BaseNPC", "m_ActivityModifiers");
    if (!MyWearablesOffset || !NetworkActivityOffset || !NetworkSequenceIndexOffset || !ActivityModifiersOffset)
    {
        LogE("C_DOTA_BaseNPC schema fields missing m_hMyWearables 0x%X m_NetworkActivity 0x%X m_NetworkSequenceIndex 0x%X m_ActivityModifiers 0x%X",
             MyWearablesOffset, NetworkActivityOffset, NetworkSequenceIndexOffset, ActivityModifiersOffset);
        return false;
    }
    return true;
}

std::vector<CHandle> C_DOTA_BaseNPC::MyWearables() const
{
    std::uintptr_t Vector = reinterpret_cast<std::uintptr_t>(this) + MyWearablesOffset;
    std::int32_t Count = Memory::Read<std::int32_t>(Vector + Offsets::CUtlVector::Size);
    auto* Data = Memory::Read<const std::uint32_t*>(Vector + Offsets::CUtlVector::Data);

    std::vector<CHandle> Out;
    if (Count <= 0 || !Data)
        return Out;

    Out.reserve(Count);
    for (std::int32_t I = 0; I < Count; ++I)
        Out.push_back(CHandle {Data[I]});
    return Out;
}


std::int32_t C_DOTA_BaseNPC::NetworkActivity() const
{
    return Memory::Read<std::int32_t>(reinterpret_cast<std::uintptr_t>(this) + NetworkActivityOffset);
}

std::int32_t C_DOTA_BaseNPC::NetworkSequenceIndex() const
{
    return Memory::Read<std::int32_t>(reinterpret_cast<std::uintptr_t>(this) + NetworkSequenceIndexOffset);
}

void C_DOTA_BaseNPC::SetNetworkSequenceIndex(std::int32_t Variant)
{
    *reinterpret_cast<std::int32_t*>(reinterpret_cast<std::uintptr_t>(this) + NetworkSequenceIndexOffset) = Variant;
}

std::int32_t C_DOTA_BaseNPC::SelectWeightedSequence(std::int32_t Activity)
{
    std::uintptr_t Modifiers = reinterpret_cast<std::uintptr_t>(this) + ActivityModifiersOffset;
    std::int32_t Count = Memory::Read<std::int32_t>(Modifiers + Offsets::CUtlVector::Size);
    void* Data = Memory::Read<void*>(Modifiers + Offsets::CUtlVector::Data);
    std::int32_t Sequence = -1;
    using SelectFn = std::int32_t* (*)(C_DOTA_BaseNPC*, std::int32_t*, std::int32_t, void*, std::int32_t);
    auto** Vtable = Memory::Read<void**>(reinterpret_cast<std::uintptr_t>(this));
    reinterpret_cast<SelectFn>(Vtable[Offsets::C_DOTA_BaseNPC::SelectWeightedSequenceIndex])(this, &Sequence, Activity, Data, Count);
    return Sequence;
}
