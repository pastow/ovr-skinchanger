#pragma once

#include "SDK/EntitySystem/CHandle.h"

class CEntityIdentity;
class C_BaseEntity;

class CGameEntitySystem
{
public:
    static constexpr int MaxEntities = 0x8000;

    static bool Init();
    static CGameEntitySystem* Get();

    CEntityIdentity* Identity(int Index) const;
    C_BaseEntity* Entity(int Index) const;
    C_BaseEntity* Entity(CHandle Handle) const;

    template <typename T>
    T* Entity(CHandle Handle) const
    {
        return reinterpret_cast<T*>(Entity(Handle));
    }
};
