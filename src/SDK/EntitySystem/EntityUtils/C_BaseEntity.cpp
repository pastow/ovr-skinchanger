#include "C_BaseEntity.h"

#include "Misc/Offsets.h"
#include "Misc/Patterns.h"
#include "SDK/EntitySystem/CEntityIdentity.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"
#include "SDK/SchemaSystem/SchemaOffset.h"

#include <cstdint>

namespace
{
    std::uint32_t EntityOffset = 0;
    std::uint32_t BodyComponentOffset = 0;
    std::uint32_t SkeletonInstanceOffset = 0;
    std::uint32_t ModelStateOffset = 0;
    std::uint32_t ModelOffset = 0;
    std::uint32_t OwnerEntityOffset = 0;
    std::uint32_t SceneNodeOffset = 0;
    std::uint32_t AbsOriginOffset = 0;
    void* (*CreateParticleFn)(void*, const char*, int, bool, int) = nullptr;
}

bool C_BaseEntity::Init()
{
    EntityOffset = SchemaOffset::Instance()->GetOffset("CEntityInstance", "m_pEntity");
    SchemaOffset* Schema = SchemaOffset::Instance();
    BodyComponentOffset = Schema->GetOffset("C_BaseEntity", "m_CBodyComponent");
    SkeletonInstanceOffset = Schema->GetOffset("CBodyComponentSkeletonInstance", "m_skeletonInstance");
    ModelStateOffset = Schema->GetOffset("CSkeletonInstance", "m_modelState");
    ModelOffset = Schema->GetOffset("CModelState", "m_hModel");
    OwnerEntityOffset = Schema->GetOffset("C_BaseEntity", "m_hOwnerEntity");
    CreateParticleFn = reinterpret_cast<decltype(CreateParticleFn)>(Memory::FindPattern("client.dll", Patterns::ParticlePropertyCreate));
    if (!CreateParticleFn)
        LogW("ParticlePropertyCreate pattern not found");
    SceneNodeOffset = Schema->GetOffset("C_BaseEntity", "m_pGameSceneNode");
    AbsOriginOffset = Schema->GetOffset("CGameSceneNode", "m_vecAbsOrigin");
    if (!SceneNodeOffset || !AbsOriginOffset)
        LogW("Scene node schema fields missing m_pGameSceneNode 0x%X m_vecAbsOrigin 0x%X", SceneNodeOffset, AbsOriginOffset);
    if (!EntityOffset || !BodyComponentOffset || !SkeletonInstanceOffset || !ModelStateOffset || !ModelOffset || !OwnerEntityOffset)
    {
        LogE("C_BaseEntity schema fields missing m_pEntity 0x%X m_CBodyComponent 0x%X m_skeletonInstance 0x%X m_modelState 0x%X m_hModel 0x%X m_hOwnerEntity 0x%X",
             EntityOffset, BodyComponentOffset, SkeletonInstanceOffset, ModelStateOffset, ModelOffset, OwnerEntityOffset);
        return false;
    }
    return true;
}

CEntityIdentity* C_BaseEntity::Identity() const
{
    return Memory::Read<CEntityIdentity*>(reinterpret_cast<std::uintptr_t>(this) + EntityOffset);
}

CHandle C_BaseEntity::Handle() const
{
    CEntityIdentity* Id = Identity();
    return Id ? Id->Handle() : CHandle {};
}

int C_BaseEntity::Index() const
{
    CHandle H = Handle();
    return H.IsValid() ? static_cast<int>(H.Index()) : -1;
}

const char* C_BaseEntity::DesignerName() const
{
    CEntityIdentity* Id = Identity();
    return Id ? Id->DesignerName() : nullptr;
}

namespace
{
    std::uintptr_t ModelBinding(const C_BaseEntity* Entity)
    {
        std::uintptr_t Body = Memory::Read<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(Entity) + BodyComponentOffset);
        return Body ? Memory::Read<std::uintptr_t>(Body + SkeletonInstanceOffset + ModelStateOffset + ModelOffset) : 0;
    }
}

std::uint64_t C_BaseEntity::ModelId() const
{
    std::uintptr_t Binding = ModelBinding(this);
    return Binding ? Memory::Read<std::uint64_t>(Binding + Offsets::ResourceBinding::Id) : 0;
}


CHandle C_BaseEntity::OwnerEntity() const
{
    return CHandle {Memory::Read<std::uint32_t>(reinterpret_cast<std::uintptr_t>(this) + OwnerEntityOffset)};
}

bool C_BaseEntity::Origin(float Out[3]) const
{
    if (!SceneNodeOffset || !AbsOriginOffset)
        return false;
    std::uintptr_t Node = Memory::Read<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(this) + SceneNodeOffset);
    if (!Node)
        return false;
    for (int Axis = 0; Axis < 3; ++Axis)
        Out[Axis] = Memory::Read<float>(Node + AbsOriginOffset + sizeof(float) * Axis);
    return true;
}

void* C_BaseEntity::CreateParticle(const char* Name, EParticleAttach Attach)
{
    if (!CreateParticleFn || !Name || !*Name)
        return nullptr;
    void* Property = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(this) + Offsets::C_BaseEntity::ParticleProperty);
    return CreateParticleFn(Property, Name, static_cast<int>(Attach), false, 0);
}
