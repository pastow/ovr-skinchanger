#include "AssetPrecache.h"

#include "Changer/Items/FakeLoadout.h"
#include "SDK/Logger/Logger.h"
#include "SDK/NetDelta/ResourceId.h"
#include "SDK/ResourceSystem/CResourceSystem.h"

#include <mutex>
#include <set>
#include <vector>

namespace
{
    constexpr const char* ManifestName = "ovr_skins";

    std::mutex PrecacheMutex;
    std::vector<CResourceManifest*> Manifests;
    std::set<std::string> Requested;

    bool EndsWith(const std::string& Text, const char* Suffix)
    {
        std::size_t Length = std::char_traits<char>::length(Suffix);
        return Text.size() > Length && Text.compare(Text.size() - Length, Length, Suffix) == 0;
    }

    bool IsAsset(const std::string& Path)
    {
        return EndsWith(Path, ".vmdl") || EndsWith(Path, ".vpcf");
    }
}

void AssetPrecache::LoadEquipped()
{
    CResourceSystem* Resources = CResourceSystem::Get();
    if (!Resources)
        return;

    std::set<std::string> Paths;
    for (const auto& Modifier : FakeLoadout::EquippedModifiers())
        if (IsAsset(Modifier.Modifier))
            Paths.insert(Modifier.Modifier);
    for (const std::string& Model : FakeLoadout::EquippedModels())
        if (IsAsset(Model))
            Paths.insert(Model);

    std::vector<const char*> Names;
    Names.reserve(Paths.size());
    for (const std::string& Path : Paths)
        Names.push_back(Path.c_str());

    CResourceManifest* Manifest = Names.empty() ? nullptr : Resources->Load(Names, ManifestName, true);

    std::lock_guard<std::mutex> Lock(PrecacheMutex);
    for (CResourceManifest* Old : Manifests)
        Resources->Release(Old);
    Manifests.clear();
    Requested = Paths;
    if (Manifest)
        Manifests.push_back(Manifest);
    std::size_t Loaded = 0;
    for (const std::string& Path : Paths)
    {
        if (Resources->Status(NetDelta::ResourceId(Path.c_str())) == CResourceSystem::EStatus::Loaded)
            ++Loaded;
        else
            LogW("Precache not loaded %s", Path.c_str());
    }
    Log("Precache equipped assets %zu loaded %zu manifest %p", Paths.size(), Loaded, static_cast<void*>(Manifest));
}

void AssetPrecache::Ensure(const std::string& Path)
{
    CResourceSystem* Resources = CResourceSystem::Get();
    if (!Resources || Path.empty())
        return;

    std::lock_guard<std::mutex> Lock(PrecacheMutex);
    if (!Requested.insert(Path).second)
        return;
    if (CResourceManifest* Manifest = Resources->Load({Path.c_str()}, ManifestName, false))
        Manifests.push_back(Manifest);
    Log("Precache %s", Path.c_str());
}
