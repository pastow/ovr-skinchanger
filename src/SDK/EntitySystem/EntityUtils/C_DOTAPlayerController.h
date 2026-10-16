#pragma once

#include "SDK/EntitySystem/EntityUtils/C_BaseEntity.h"

class C_DOTA_BaseNPC_Hero;

class C_DOTAPlayerController : public C_BaseEntity
{
public:
    static bool Init();
    static C_DOTAPlayerController* GetLocal();
    static C_DOTA_BaseNPC_Hero* LocalHero();

    CHandle AssignedHero() const;
};
