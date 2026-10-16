#include "EconWearableSpawnHook.h"

#include "Changer/Match/WearableSwap.h"
#include "Misc/Offsets.h"
#include "Misc/Patterns.h"
#include "SDK/Econ/C_EconItemView.h"
#include "SDK/EntitySystem/EntityUtils/C_BaseEntity.h"
#include "SDK/GameRules/C_DOTAGamerules.h"
#include "SDK/KeyValues/CEntityKeyValues.h"
#include "SDK/KeyValues/KeyValues3.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <MinHook.h>

#include <cstdint>

namespace
{
    constexpr std::uint32_t ItemDataHash = 0x40D37573;
    constexpr const char* ItemDataName = "ItemData";

    void (*Original)(void*, void*) = nullptr;

    void Detour(void* Wearable, void* SpawnInfo)
    {
        alignas(16) std::uint8_t Storage[Offsets::C_EconItemView::Size];
        KeyValues3* Value = nullptr;
        void* OriginalView = nullptr;
        auto* FakeView = reinterpret_cast<C_EconItemView*>(Storage);

        auto* Kv = SpawnInfo ? Memory::Read<CEntityKeyValues*>(reinterpret_cast<std::uintptr_t>(SpawnInfo) + Offsets::EntitySpawnInfo::KeyValues) : nullptr;
        if (Kv && C_DOTAGamerules::Get())
        {
            KeyValues3* Found = Kv->FindValue(ItemDataHash, ItemDataName);
            auto* View = Found && Found->HoldsPointer() ? static_cast<C_EconItemView*>(Found->Pointer()) : nullptr;
            if (View)
            {
                if (auto Swap = WearableSwap::Find(View))
                {
                    C_EconItemView::Construct(FakeView);
                    FakeView->Initialize(Swap->DefIndex, Swap->AccountId, Swap->Style);
                    FakeView->SetItemId(Swap->ItemId);
                    Value = Found;
                    OriginalView = View;
                    Value->SetPointer(FakeView);
                }
            }
        }

        Original(Wearable, SpawnInfo);

        if (Value)
        {
            Value->SetPointer(OriginalView);
            FakeView->Destruct();
        }
    }
}

bool EconWearableSpawnHook::Install()
{
    std::uintptr_t Target = Memory::FindPattern("client.dll", Patterns::EconWearableSpawn);
    if (!Target)
    {
        LogE("EconWearableSpawn pattern not found");
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
        LogE("EconWearableSpawn hook failed: %s", MH_StatusToString(Status));
        return false;
    }

    Log("EconWearableSpawn %#llx", static_cast<unsigned long long>(Target));
    return true;
}
