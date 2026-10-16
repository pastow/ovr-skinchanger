#pragma once

#include "SDK/EntitySystem/EntityUtils/C_DOTA_BaseNPC.h"

#include <cstdint>

class C_DOTA_BaseNPC_Hero : public C_DOTA_BaseNPC
{
public:
    static bool Init();

    std::int32_t PlayerID() const;
    std::uint32_t HeroId() const;
    CHandle ReplicatingOtherHeroModel() const;
    bool IsIllusion() const;
};
