#include "CEntityIdentity.h"

#include "Misc/Offsets.h"
#include "SDK/Memory/Memory.h"

C_BaseEntity* CEntityIdentity::Entity() const
{
    return Memory::Read<C_BaseEntity*>(reinterpret_cast<std::uintptr_t>(this) + Offsets::CEntityIdentity::Entity);
}

CHandle CEntityIdentity::Handle() const
{
    return CHandle {Memory::Read<std::uint32_t>(reinterpret_cast<std::uintptr_t>(this) + Offsets::CEntityIdentity::Handle)};
}

const char* CEntityIdentity::Name() const
{
    return Memory::Read<const char*>(reinterpret_cast<std::uintptr_t>(this) + Offsets::CEntityIdentity::Name);
}

const char* CEntityIdentity::DesignerName() const
{
    return Memory::Read<const char*>(reinterpret_cast<std::uintptr_t>(this) + Offsets::CEntityIdentity::DesignerName);
}
