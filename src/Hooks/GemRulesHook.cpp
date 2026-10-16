#include "GemRulesHook.h"

#include "Changer/Inventory/FakeItems.h"
#include "Misc/Patterns.h"
#include "SDK/Econ/IEconItemInterface.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <MinHook.h>

#include <cstdint>

namespace
{
    constexpr int RuleOk = 0;

    int (*Original)(void*, IEconItemInterface*, std::uint32_t, IEconItemInterface*) = nullptr;

    int Detour(void* Utils, IEconItemInterface* Target, std::uint32_t SocketType, IEconItemInterface* Gem)
    {
        int Result = Original(Utils, Target, SocketType, Gem);
        if (Result == RuleOk || !Target || !Gem)
            return Result;
        std::uint64_t TargetId = Target->ItemId();
        if (!FakeItems::IsFake(TargetId))
            return Result;
        return RuleOk;
    }
}

bool GemRulesHook::Install()
{
    std::uintptr_t Target = Memory::FindPattern("client.dll", Patterns::GemRules);
    if (!Target)
    {
        LogE("GemRules pattern not found");
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
        LogE("GemRules hook failed: %s", MH_StatusToString(Status));
        return false;
    }

    return true;
}
