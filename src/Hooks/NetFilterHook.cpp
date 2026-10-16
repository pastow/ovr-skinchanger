#include "NetFilterHook.h"

#include "Changer/Match/PacketRewriter.h"
#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"
#include "SDK/Network/CNetMessage.h"

#include <MinHook.h>

#include <cstdint>

namespace
{
    bool (*Original)(void*, CNetMessage*, void*) = nullptr;

    bool Detour(void* Filter, CNetMessage* Message, void* Channel)
    {
        PacketRewriter::OnMessage(Message);
        return Original(Filter, Message, Channel);
    }
}

bool NetFilterHook::Install()
{
    std::uintptr_t Target = Memory::FindPattern("engine2.dll", Patterns::NetFilter);
    if (!Target)
    {
        LogE("NetFilter pattern not found");
        return false;
    }

    MH_STATUS Status = MH_Initialize();
    if (Status != MH_OK && Status != MH_ERROR_ALREADY_INITIALIZED)
    {
        LogE("MinHook init failed: %s", MH_StatusToString(Status));
        return false;
    }

    auto* TargetFn = reinterpret_cast<void*>(Target);
    Status = MH_CreateHook(TargetFn, reinterpret_cast<void*>(Detour), reinterpret_cast<void**>(&Original));
    if (Status == MH_OK)
        Status = MH_EnableHook(TargetFn);

    if (Status != MH_OK)
    {
        LogE("NetFilter hook failed: %s", MH_StatusToString(Status));
        return false;
    }

    Log("NetFilter %#llx", static_cast<unsigned long long>(Target));
    return true;
}
