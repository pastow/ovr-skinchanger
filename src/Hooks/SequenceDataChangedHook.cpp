#include "SequenceDataChangedHook.h"

#include "Changer/Animation/AnimationFix.h"
#include "Changer/Animation/AnimationLogger.h"
#include "Misc/Patterns.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTA_BaseNPC.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <MinHook.h>

#include <cstdint>

namespace
{
    void (*Original)(C_DOTA_BaseNPC*, bool) = nullptr;

    void Detour(C_DOTA_BaseNPC* Npc, bool ResetCycle)
    {
        AnimationFix::Apply(Npc);
        AnimationLogger::OnSequenceChanged(Npc);
        Original(Npc, ResetCycle);
    }
}

bool SequenceDataChangedHook::Install()
{
    std::uintptr_t Target = Memory::FindPattern("client.dll", Patterns::SequenceDataChanged);
    if (!Target)
    {
        LogE("SequenceDataChanged pattern not found");
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
        LogE("SequenceDataChanged hook failed: %s", MH_StatusToString(Status));
        return false;
    }

    Log("SequenceDataChanged %#llx", static_cast<unsigned long long>(Target));
    return true;
}
