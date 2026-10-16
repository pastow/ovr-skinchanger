#pragma once

#include "SDK/EntitySystem/CHandle.h"

#include <cstdint>

class CEntityIdentity;

class C_BaseEntity
{
public:
    enum class EParticleAttach : int
    {
        AbsOrigin = 0,
        AbsOriginFollow = 1
    };

    static bool Init();

    CEntityIdentity* Identity() const;
    CHandle Handle() const;
    int Index() const;
    const char* DesignerName() const;
    std::uint64_t ModelId() const;
    CHandle OwnerEntity() const;
    bool Origin(float Out[3]) const;
    void* CreateParticle(const char* Name, EParticleAttach Attach);
};
