#include "EmblemEffect.h"

#include "Changer/Items/AssetPrecache.h"
#include "Changer/Items/FakeLoadout.h"
#include "Changer/Match/NetSkins.h"
#include "Changer/Match/ImpactEffects.h"
#include "Changer/Match/ParticleReplace.h"
#include "SDK/EntitySystem/CGameEntitySystem.h"
#include "SDK/EntitySystem/EntityUtils/C_BaseEntity.h"
#include "SDK/Logger/Logger.h"
#include "SDK/NetDelta/ResourceId.h"
#include "SDK/Network/CNetMessage.h"
#include "SDK/Protobuf/CProtobufMessage.h"
#include "SDK/ResourceSystem/CResourceSystem.h"

#include "dota_usermessages.pb.h"

#include <atomic>
#include <type_traits>
#include <mutex>
#include <string>

namespace
{
    constexpr const char* EmblemType = "diretide_effect";
    constexpr const char* EmblemAsset = "emblem_attack_effect";
    constexpr std::uint32_t NetworkIndexMask = 0x3FFF;
    constexpr int ServerAdditionalLogLimit = 200;

    std::mutex PathMutex;
    std::string AttackPath;
    std::atomic<int> ServerAdditionalLogs{0};

    std::string CurrentPath()
    {
        std::lock_guard<std::mutex> Lock(PathMutex);
        return AttackPath;
    }

    template <typename T>
    void Apply(CNetMessage* Message)
    {
        CProtobufMessage* Proto = Message->Proto();
        T Projectile;
        if (!Proto || !Proto->CopyTo(Projectile))
            return;

        bool Own = NetSkins::IsOwnHero(static_cast<int>(Projectile.source() & NetworkIndexMask));
        bool Changed = false;
        int Source = static_cast<int>(Projectile.source() & NetworkIndexMask);
        std::uint64_t Particle = static_cast<std::uint64_t>(Projectile.particle_system_handle());
        CGameEntitySystem* Entities = CGameEntitySystem::Get();
        C_BaseEntity* SourceEntity = Entities ? Entities->Entity(Source) : nullptr;
        bool Swapped = Projectile.is_attack() && SourceEntity && NetSkins::IsSwapped(Source, SourceEntity->ModelId());
        if (Swapped && ParticleReplace::Replace(Particle))
        {
            Projectile.set_particle_system_handle(static_cast<std::int64_t>(Particle));
            Changed = true;
        }
        if constexpr (std::is_same_v<T, CDOTAUserMsg_TE_Projectile>)
        {
            if (Swapped)
                ImpactEffects::OnAttackProjectile(Source, Projectile.handle(), Projectile.target());
        }

        if (Projectile.has_additional_particle_system_handle() && Projectile.additional_particle_system_handle())
        {
            if (ServerAdditionalLogs.fetch_add(1) < ServerAdditionalLogLimit)
            {
                CResourceSystem* Resources = CResourceSystem::Get();
                const char* Name = Resources ? Resources->Name(static_cast<std::uint64_t>(Projectile.additional_particle_system_handle())) : nullptr;
                Log("Emblem server additional %s source %u", Name ? Name : "?", Projectile.source());
            }
        }
        else if (Own && Projectile.is_attack())
        {
            std::string Path = CurrentPath();
            if (!Path.empty())
            {
                AssetPrecache::Ensure(Path);
                Projectile.set_additional_particle_system_handle(static_cast<std::int64_t>(NetDelta::ResourceId(Path.c_str())));
                Changed = true;
            }
        }

        if (!Changed)
            return;
        if (!Proto->CopyFrom(Projectile))
            LogE("Projectile write back failed");
    }
}

void EmblemEffect::Update()
{
    std::string Path = FakeLoadout::Find(FakeLoadout::EquippedModifiers(), EmblemType, EmblemAsset);
    std::lock_guard<std::mutex> Lock(PathMutex);
    if (Path != AttackPath)
        Log("Emblem attack effect %s", Path.empty() ? "none" : Path.c_str());
    AttackPath = std::move(Path);
}

void EmblemEffect::OnProjectile(CNetMessage* Message, bool Located)
{
    if (!Message)
        return;
    if (Located)
        Apply<CDOTAUserMsg_TE_ProjectileLoc>(Message);
    else
        Apply<CDOTAUserMsg_TE_Projectile>(Message);
}
