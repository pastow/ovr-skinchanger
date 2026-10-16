#pragma once

class C_DOTA_BaseNPC;
class CNetMessage;

namespace AnimationLogger
{
    void OnSequenceChanged(C_DOTA_BaseNPC* Npc);
    void OnUnitAnimation(CNetMessage* Message);
    void OnProjectile(CNetMessage* Message, bool Located);
    void OnParticleManager(CNetMessage* Message);
}
