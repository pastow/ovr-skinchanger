#pragma once

#include "SDK/EntitySystem/CHandle.h"

class C_BaseEntity;

class CEntityIdentity
{
public:
    C_BaseEntity* Entity() const;
    CHandle Handle() const;
    const char* Name() const;
    const char* DesignerName() const;
};
