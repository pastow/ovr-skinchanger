#include "CGCClient.h"

#include "Misc/Offsets.h"
#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <cstdint>

namespace
{
    std::uintptr_t ClientSystem = 0;
}

bool CGCClient::Init()
{
    std::uintptr_t Site = Memory::FindPattern("client.dll", Patterns::GCClientSystem);
    if (!Site)
    {
        LogE("GCClientSystem pattern not found");
        return false;
    }
    ClientSystem = Memory::ResolveRelative(Site, 3, 7);
    Log("CGCClient %p", static_cast<void*>(Get()));
    return true;
}

CGCClient* CGCClient::Get()
{
    return ClientSystem ? reinterpret_cast<CGCClient*>(ClientSystem + Offsets::CGCClientSystem::GCClient) : nullptr;
}

void* CGCClient::SteamGameCoordinator() const
{
    return Memory::Read<void*>(reinterpret_cast<std::uintptr_t>(this) + Offsets::CGCClient::SteamGameCoordinator);
}
