#include "ImpactEffects.h"

#include "Changer/Items/AssetPrecache.h"
#include "Changer/Items/FakeLoadout.h"
#include "SDK/EntitySystem/CGameEntitySystem.h"
#include "SDK/EntitySystem/EntityUtils/C_BaseEntity.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Network/CNetMessage.h"
#include "SDK/Protobuf/CProtobufMessage.h"

#include "dota_usermessages.pb.h"

#include <atomic>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

namespace
{
    constexpr std::uint32_t NetworkIndexMask = 0x3FFF;
    constexpr const char* ParticleType = "particle";
    constexpr std::size_t MaxPending = 64;
    constexpr int LogLimit = 100;

    struct ArcanaImpact
    {
        const char* UnitName;
        const char* Particle;
    };

    constexpr ArcanaImpact Impacts[] = {
        { "npc_dota_hero_razor", "particles/econ/items/razor/razor_arcana/razor_arcana_base_attack_impact.vpcf" },
    };

    struct Pending
    {
        int Target;
        std::string Particle;
    };

    std::mutex ImpactMutex;
    std::unordered_map<std::int32_t, Pending> Projectiles;
    std::deque<Pending> Queue;
    std::atomic<int> Logs{0};

    const char* ImpactFor(const char* UnitName)
    {
        if (!UnitName)
            return nullptr;
        for (const ArcanaImpact& Entry : Impacts)
            if (std::strcmp(Entry.UnitName, UnitName) == 0)
                return Entry.Particle;
        return nullptr;
    }
}

void ImpactEffects::OnAttackProjectile(int Source, std::int32_t Handle, std::uint32_t Target)
{
    CGameEntitySystem* Entities = CGameEntitySystem::Get();
    C_BaseEntity* Hero = Entities ? Entities->Entity(Source) : nullptr;
    const char* Impact = Hero ? ImpactFor(Hero->DesignerName()) : nullptr;
    if (!Impact)
        return;

    std::string Particle = FakeLoadout::Find(FakeLoadout::EquippedModifiers(), ParticleType, Impact);
    if (Particle.empty())
        Particle = Impact;
    AssetPrecache::Ensure(Particle);

    std::lock_guard<std::mutex> Lock(ImpactMutex);
    if (Projectiles.size() >= MaxPending)
        Projectiles.clear();
    Projectiles[Handle] = {static_cast<int>(Target & NetworkIndexMask), std::move(Particle)};
}

void ImpactEffects::OnDestroyProjectile(CNetMessage* Message)
{
    CProtobufMessage* Proto = Message ? Message->Proto() : nullptr;
    CDOTAUserMsg_TE_DestroyProjectile Destroy;
    if (!Proto || !Proto->CopyTo(Destroy))
        return;

    std::lock_guard<std::mutex> Lock(ImpactMutex);
    auto It = Projectiles.find(Destroy.handle());
    if (It == Projectiles.end())
        return;
    if (Queue.size() < MaxPending)
        Queue.push_back(std::move(It->second));
    Projectiles.erase(It);
}

void ImpactEffects::Update()
{
    std::deque<Pending> Ready;
    {
        std::lock_guard<std::mutex> Lock(ImpactMutex);
        Ready.swap(Queue);
    }
    CGameEntitySystem* Entities = CGameEntitySystem::Get();
    for (const Pending& Item : Ready)
    {
        C_BaseEntity* Target = Entities ? Entities->Entity(Item.Target) : nullptr;
        void* Effect = Target ? Target->CreateParticle(Item.Particle.c_str(), C_BaseEntity::EParticleAttach::AbsOriginFollow) : nullptr;
        if (Logs.fetch_add(1) < LogLimit)
            Log("Impact %s on %d -> %p", Item.Particle.c_str(), Item.Target, Effect);
    }
}
