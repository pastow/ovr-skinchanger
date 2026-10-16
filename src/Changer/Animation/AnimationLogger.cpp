#include "AnimationLogger.h"

#include "Changer/Match/NetSkins.h"
#include "SDK/EntitySystem/CGameEntitySystem.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTA_BaseNPC.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Network/CNetMessage.h"
#include "SDK/Protobuf/CProtobufMessage.h"
#include "SDK/ResourceSystem/CModel.h"
#include "SDK/ResourceSystem/CResourceSystem.h"
#include "SDK/SchemaSystem/SchemaOffset.h"
#include "SDK/Memory/Memory.h"

#include <cmath>
#include <string>

#include "dota_usermessages.pb.h"
#include "usermessages.pb.h"

#include <atomic>

namespace
{
    constexpr std::uint32_t NetworkIndexMask = 0x3FFF;
    constexpr int LogLimit = 3000;

    std::atomic<int> Logs{0};

    bool Allow()
    {
        return Logs.fetch_add(1) < LogLimit;
    }

    bool IsOwn(std::uint32_t Handle)
    {
        return NetSkins::IsOwnHero(static_cast<int>(Handle & NetworkIndexMask));
    }

    const char* ResourceName(std::uint64_t Hash)
    {
        CResourceSystem* Resources = CResourceSystem::Get();
        const char* Name = Hash && Resources ? Resources->Name(Hash) : nullptr;
        return Name ? Name : "?";
    }

    CModel* ModelOf(C_DOTA_BaseNPC* Npc)
    {
        CResourceSystem* Resources = CResourceSystem::Get();
        return Npc && Resources ? static_cast<CModel*>(Resources->Data(Npc->ModelId())) : nullptr;
    }

    const char* SequenceName(CModel* Model, int Sequence)
    {
        const char* Name = Model && Sequence >= 0 ? Model->SequenceName(Sequence) : nullptr;
        return Name ? Name : "?";
    }

    const char* DefaultSequenceName(int Index, int Activity, int Variant)
    {
        CResourceSystem* Resources = CResourceSystem::Get();
        std::uint64_t Default = NetSkins::DefaultModelOf(Index);
        auto* Model = Default && Resources ? static_cast<CModel*>(Resources->Data(Default)) : nullptr;
        return Model ? SequenceName(Model, Model->SequenceForVariant(Activity, Variant)) : "-";
    }

    std::uint32_t AttackTargetOffset = 0;
    bool AttackTargetResolved = false;

    std::string TargetInfo(C_DOTA_BaseNPC* Npc)
    {
        if (!AttackTargetResolved)
        {
            AttackTargetResolved = true;
            std::string Class;
            AttackTargetOffset = SchemaOffset::Instance()->FindField("m_hAttackTarget", Class);
            Log("AnimLog m_hAttackTarget class %s offset 0x%X", Class.c_str(), AttackTargetOffset);
        }
        CGameEntitySystem* Entities = CGameEntitySystem::Get();
        if (!Npc || !AttackTargetOffset || !Entities)
            return "target ?";
        CHandle Target {Memory::Read<std::uint32_t>(reinterpret_cast<std::uintptr_t>(Npc) + AttackTargetOffset)};
        C_BaseEntity* Entity = Entities->Entity(Target);
        float From[3];
        float To[3];
        if (!Entity || !Npc->Origin(From) || !Entity->Origin(To))
            return "target " + std::to_string(Target.IsValid() ? static_cast<int>(Target.Index()) : -1);
        float Distance = std::sqrt((From[0] - To[0]) * (From[0] - To[0]) + (From[1] - To[1]) * (From[1] - To[1]));
        return "target " + std::to_string(Target.Index()) + " distance " + std::to_string(static_cast<int>(Distance));
    }
}

void AnimationLogger::OnSequenceChanged(C_DOTA_BaseNPC* Npc)
{
    if (!Npc || !NetSkins::IsOwnHero(Npc->Index()) || !Allow())
        return;
    CModel* Model = ModelOf(Npc);
    int Activity = Npc->NetworkActivity();
    int Variant = Npc->NetworkSequenceIndex();
    int Sequence = Model ? Model->SequenceForVariant(Activity, Variant) : -1;
    Log("AnimLog seq npc %d model %s activity %d variant %d sequence %d %s default %s", Npc->Index(), ResourceName(Npc->ModelId()), Activity, Variant,
        Sequence, SequenceName(Model, Sequence), DefaultSequenceName(Npc->Index(), Activity, Variant));
}

void AnimationLogger::OnUnitAnimation(CNetMessage* Message)
{
    CProtobufMessage* Proto = Message ? Message->Proto() : nullptr;
    CDOTAUserMsg_TE_UnitAnimation Animation;
    if (!Proto || !Proto->CopyTo(Animation) || !IsOwn(Animation.entity()) || !Allow())
        return;
    CGameEntitySystem* Entities = CGameEntitySystem::Get();
    auto* Npc = Entities ? static_cast<C_DOTA_BaseNPC*>(Entities->Entity(static_cast<int>(Animation.entity() & NetworkIndexMask))) : nullptr;
    CModel* Model = ModelOf(Npc);
    int Sequence = Model ? Model->SequenceForVariant(Animation.activity(), Animation.sequence_variant()) : -1;
    Log("AnimLog unit npc %u type %d activity %d variant %d rate %f castpoint %f lag %f sequence %d %s default %s %s", Animation.entity() & NetworkIndexMask,
        Animation.type(), Animation.activity(), Animation.sequence_variant(), static_cast<double>(Animation.playbackrate()),
        static_cast<double>(Animation.castpoint()), static_cast<double>(Animation.lag_compensation_time()), Sequence, SequenceName(Model, Sequence),
        DefaultSequenceName(static_cast<int>(Animation.entity() & NetworkIndexMask), Animation.activity(), Animation.sequence_variant()),
        TargetInfo(Npc).c_str());
}

void AnimationLogger::OnProjectile(CNetMessage* Message, bool Located)
{
    CProtobufMessage* Proto = Message ? Message->Proto() : nullptr;
    if (!Proto)
        return;
    if (Located)
    {
        CDOTAUserMsg_TE_ProjectileLoc Projectile;
        if (!Proto->CopyTo(Projectile) || !IsOwn(Projectile.source()) || !Allow())
            return;
        Log("AnimLog projectile_loc source %u attack %d particle %s additional %s", Projectile.source() & NetworkIndexMask, Projectile.is_attack() ? 1 : 0,
            ResourceName(static_cast<std::uint64_t>(Projectile.particle_system_handle())),
            ResourceName(static_cast<std::uint64_t>(Projectile.additional_particle_system_handle())));
        return;
    }
    CDOTAUserMsg_TE_Projectile Projectile;
    if (!Proto->CopyTo(Projectile) || !IsOwn(Projectile.source()) || !Allow())
        return;
    Log("AnimLog projectile source %u target %u attack %d attachment %d particle %s additional %s", Projectile.source() & NetworkIndexMask,
        Projectile.target() & NetworkIndexMask, Projectile.is_attack() ? 1 : 0, Projectile.source_attachment(),
        ResourceName(static_cast<std::uint64_t>(Projectile.particle_system_handle())),
        ResourceName(static_cast<std::uint64_t>(Projectile.additional_particle_system_handle())));
}

void AnimationLogger::OnParticleManager(CNetMessage* Message)
{
    CProtobufMessage* Proto = Message ? Message->Proto() : nullptr;
    CUserMsg_ParticleManager Particle;
    if (!Proto || !Proto->CopyTo(Particle) || !Particle.has_create_particle())
        return;
    const auto& Create = Particle.create_particle();
    if (!IsOwn(Create.entity_handle()) && !IsOwn(Create.entity_handle_for_modifiers()))
        return;
    if (!Allow())
        return;
    Log("AnimLog particle index %u entity %u modifiers %u attach %d name %s", Particle.index(), Create.entity_handle() & NetworkIndexMask,
        Create.entity_handle_for_modifiers() & NetworkIndexMask, Create.attach_type(), ResourceName(Create.particle_name_index()));
}
