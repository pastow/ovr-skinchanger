#pragma once

#include <cstdint>
#include <vector>

class CResourceManifest;

class CResourceSystem
{
public:
    enum class EStatus : std::int32_t
    {
        Absent = 1,
        Loading = 2,
        Loaded = 3
    };

    static bool Init();
    static CResourceSystem* Get();

    CResourceManifest* Load(const std::vector<const char*>& Paths, const char* DebugName, bool Blocking);
    void Release(CResourceManifest* Manifest);

    EStatus Status(std::uint64_t ResourceId);
    const char* Name(std::uint64_t ResourceId);
    void* Data(std::uint64_t ResourceId);
};
