#include <windows.h>

#include "Hooks/Hooks.h"
#include "SDK/Logger/Logger.h"
#include "SDK/SDK.h"

static DWORD WINAPI MainThread(LPVOID)
{
    if (!Logger::Init())
        return 0;

    Log("Loaded");

    if (!SDK::Init())
        return 0;

    Hooks::Install();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE Instance, DWORD Reason, LPVOID Reserved)
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(Instance);
        if (HANDLE Thread = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr))
            CloseHandle(Thread);
    }
    else if (Reason == DLL_PROCESS_DETACH)
        LogW("Detach, process exit %d", Reserved != nullptr);
    return TRUE;
}
