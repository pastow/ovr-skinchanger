#include "AnimationFix.h"

#include "Changer/Match/NetSkins.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTA_BaseNPC.h"
#include "SDK/Logger/Logger.h"
#include "SDK/ResourceSystem/CModel.h"
#include "SDK/ResourceSystem/CResourceSystem.h"
#include "SDK/EntitySystem/CGameEntitySystem.h"
#include "SDK/Network/CNetMessage.h"
#include "SDK/Protobuf/CProtobufMessage.h"

#include "dota_usermessages.pb.h"

#include <atomic>

namespace
{
    constexpr int LogLimit = 300;
    constexpr std::uint32_t NetworkIndexMask = 0x3FFF;
    constexpr std::int32_t ActivityAttack = 1503;
    constexpr const char* RangedAttackPart = "_far";

    std::atomic<int> Logs{0};
}

std::int32_t AnimationFix::VariantFor(C_DOTA_BaseNPC* Npc, std::int32_t Activity, std::int32_t* SequenceOut)
{
    if (!Npc)
        return -1;
    std::uint64_t ModelId = Npc->ModelId();
    CResourceSystem* Resources = CResourceSystem::Get();
    if (!Resources || !NetSkins::IsSwapped(Npc->Index(), ModelId))
        return -1;
    auto* Model = static_cast<CModel*>(Resources->Data(ModelId));
    if (!Model)
        return -1;
    if (Activity == ActivityAttack)
    {
        int Ranged = Model->VariantNamed(Activity, RangedAttackPart);
        if (Ranged >= 0)
        {
            if (SequenceOut)
                *SequenceOut = Model->SequenceForVariant(Activity, Ranged);
            return Ranged;
        }
    }
    std::int32_t Sequence = Npc->SelectWeightedSequence(Activity);
    if (Sequence < 0)
        return -1;
    if (SequenceOut)
        *SequenceOut = Sequence;
    return Model->VariantOf(Activity, Sequence);
}

void AnimationFix::Apply(C_DOTA_BaseNPC* Npc)
{
    if (!Npc)
        return;
    std::int32_t Activity = Npc->NetworkActivity();
    std::int32_t Sequence = -1;
    std::int32_t Variant = VariantFor(Npc, Activity, &Sequence);
    std::int32_t Server = Npc->NetworkSequenceIndex();
    if (Variant < 0 || Variant == Server)
        return;

    Npc->SetNetworkSequenceIndex(Variant);
    if (Logs.fetch_add(1) < LogLimit)
        Log("Animation npc %d activity %d variant %d -> %d sequence %d", Npc->Index(), Activity, Server, Variant, Sequence);
}

void AnimationFix::OnUnitAnimation(CNetMessage* Message)
{
    CProtobufMessage* Proto = Message ? Message->Proto() : nullptr;
    CDOTAUserMsg_TE_UnitAnimation Animation;
    CGameEntitySystem* Entities = CGameEntitySystem::Get();
    if (!Proto || !Entities || !Proto->CopyTo(Animation) || Animation.activity() != ActivityAttack)
        return;

    auto* Npc = static_cast<C_DOTA_BaseNPC*>(Entities->Entity(static_cast<int>(Animation.entity() & NetworkIndexMask)));
    CResourceSystem* Resources = CResourceSystem::Get();
    if (!Npc || !Resources || !NetSkins::IsSwapped(Npc->Index(), Npc->ModelId()))
        return;
    auto* Model = static_cast<CModel*>(Resources->Data(Npc->ModelId()));
    int Variant = Model ? Model->VariantNamed(Animation.activity(), RangedAttackPart) : -1;
    if (Variant < 0 || Variant == Animation.sequence_variant())
        return;

    std::int32_t Server = Animation.sequence_variant();
    Animation.set_sequence_variant(Variant);
    if (!Proto->CopyFrom(Animation))
    {
        LogE("Unit animation write back failed");
        return;
    }
    if (Logs.fetch_add(1) < LogLimit)
        Log("Unit animation npc %d activity %d variant %d -> %d %s", Npc->Index(), Animation.activity(), Server, Variant,
            Model->SequenceName(Model->SequenceForVariant(Animation.activity(), Variant)));
}
