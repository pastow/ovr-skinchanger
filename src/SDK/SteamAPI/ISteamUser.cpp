#include "ISteamUser.h"

#include "SDK/Logger/Logger.h"

#include <windows.h>

namespace
{
    using SteamUserFn = ISteamUser* (*)();
    using GetSteamIdFn = std::uint64_t (*)(ISteamUser*);

    SteamUserFn  SteamUser = nullptr;
    GetSteamIdFn GetSteamIdExport = nullptr;
}

bool ISteamUser::Init()
{
    HMODULE Api = GetModuleHandleA("steam_api64.dll");
    if (!Api)
    {
        LogE("steam_api64.dll not loaded");
        return false;
    }

    SteamUser = reinterpret_cast<SteamUserFn>(GetProcAddress(Api, "SteamAPI_SteamUser_v023"));
    GetSteamIdExport = reinterpret_cast<GetSteamIdFn>(GetProcAddress(Api, "SteamAPI_ISteamUser_GetSteamID"));
    if (!SteamUser || !GetSteamIdExport)
    {
        LogE("steam_api exports missing: SteamUser %p GetSteamID %p", reinterpret_cast<void*>(SteamUser),
             reinterpret_cast<void*>(GetSteamIdExport));
        return false;
    }

    Log("ISteamUser %p", static_cast<void*>(Get()));
    return true;
}

ISteamUser* ISteamUser::Get()
{
    return SteamUser ? SteamUser() : nullptr;
}

std::uint64_t ISteamUser::GetSteamID()
{
    return GetSteamIdExport ? GetSteamIdExport(this) : 0;
}

std::uint32_t ISteamUser::AccountId()
{
    return static_cast<std::uint32_t>(GetSteamID() & 0xFFFFFFFFull);
}
