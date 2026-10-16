#include "FrameStageNotifyHook.h"

#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <MinHook.h>

#include <cstdint>

namespace
{
    constexpr int UpdateStage = 8;

    void (*Original)(void*, int) = nullptr;

    void Detour(void* Client, int Stage)
    {
        Original(Client, Stage);

        if (Stage != UpdateStage)
            return;
    }
}

bool FrameStageNotifyHook::Install()
{
    std::uintptr_t Target = Memory::FindPattern("client.dll", Patterns::FrameStageNotify);
    if (!Target)
    {
        LogE("FrameStageNotify pattern not found");
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
        LogE("FrameStageNotify hook failed: %s", MH_StatusToString(Status));
        return false;
    }

    Log("FrameStageNotify %#llx", static_cast<unsigned long long>(Target));
    return true;
}
