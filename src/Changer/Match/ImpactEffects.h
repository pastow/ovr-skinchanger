#pragma once

#include <cstdint>

class CNetMessage;

namespace ImpactEffects
{
    void OnAttackProjectile(int Source, std::int32_t Handle, std::uint32_t Target);
    void OnDestroyProjectile(CNetMessage* Message);
    void Update();
}
