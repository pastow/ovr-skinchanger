#pragma once

#include <cstdint>

class C_DOTA_BaseNPC;
class CNetMessage;

namespace AnimationFix
{
    std::int32_t VariantFor(C_DOTA_BaseNPC* Npc, std::int32_t Activity, std::int32_t* SequenceOut);
    void Apply(C_DOTA_BaseNPC* Npc);
    void OnUnitAnimation(CNetMessage* Message);
}
