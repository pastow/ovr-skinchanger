#include "GcSendHook.h"

#include "Changer/Gc/EquipItems.h"
#include "Changer/Gc/ItemStyles.h"
#include "Changer/Gc/Sockets.h"
#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <MinHook.h>

#include <cstdint>

namespace
{
    bool (*Original)(void*, std::uint32_t, const void*, std::uint32_t) = nullptr;

    constexpr std::uint32_t EMsgExtractGems = 1086;
    constexpr std::uint32_t EMsgAddSocket = 1087;
    constexpr std::uint32_t EMsgAddItemToSocket = 1088;
    constexpr std::uint32_t EMsgEquipItems = 2569;
    constexpr std::uint32_t EMsgSetItemStyle = 2577;

    bool Handle(std::uint32_t EMsg, const void* Data, std::uint32_t Size)
    {
        switch (EMsg)
        {
        case EMsgEquipItems:
            return EquipItems::OnRequest(Data, Size);
        case EMsgSetItemStyle:
            return ItemStyles::OnSetStyle(Data, Size);
        case EMsgAddSocket:
            return Sockets::OnAddSocket(Data, Size);
        case EMsgAddItemToSocket:
            return Sockets::OnAddGem(Data, Size);
        case EMsgExtractGems:
            return Sockets::OnExtractGems(Data, Size);
        default:
            return false;
        }
    }

    bool Detour(void* Client, std::uint32_t EMsg, const void* Data, std::uint32_t Size)
    {
        if (Handle(EMsg & 0x7FFFFFFFu, Data, Size))
            return true;
        return Original(Client, EMsg, Data, Size);
    }
}

bool GcSendHook::Install()
{
    std::uintptr_t Target = Memory::FindPattern("client.dll", Patterns::GcSend);
    if (!Target)
    {
        LogE("GcSend pattern not found");
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
        LogE("GcSend hook failed: %s", MH_StatusToString(Status));
        return false;
    }

    Log("GcSend %#llx", static_cast<unsigned long long>(Target));
    return true;
}
