#include "ParticleReplace.h"

#include "Changer/Items/AssetPrecache.h"
#include "Changer/Items/FakeLoadout.h"
#include "SDK/Logger/Logger.h"
#include "SDK/NetDelta/ResourceId.h"
#include "SDK/ResourceSystem/CResourceSystem.h"

#include <atomic>
#include <string>

namespace
{
    constexpr const char* ParticleType = "particle";
    constexpr int LogLimit = 100;

    std::atomic<int> Logs{0};
}

bool ParticleReplace::Replace(std::uint64_t& Hash)
{
    CResourceSystem* Resources = CResourceSystem::Get();
    const char* Name = Hash && Resources ? Resources->Name(Hash) : nullptr;
    if (!Name)
        return false;

    std::string Modifier = FakeLoadout::Find(FakeLoadout::EquippedModifiers(), ParticleType, Name);
    if (Modifier.empty() || Modifier == Name)
        return false;

    AssetPrecache::Ensure(Modifier);
    Hash = NetDelta::ResourceId(Modifier.c_str());
    if (Logs.fetch_add(1) < LogLimit)
        Log("Particle %s -> %s", Name, Modifier.c_str());
    return true;
}
