#include "Hooks.h"

#include "EconWearableSpawnHook.h"
#include "FrameStageNotifyHook.h"
#include "GcRecvHook.h"
#include "GcSendHook.h"
#include "GemRulesHook.h"
#include "ItemInLoadoutHook.h"
#include "NetFilterHook.h"
#include "SequenceDataChangedHook.h"
#include "RunFrameHook.h"

bool Hooks::Install()
{
    bool Ok = true;
    Ok &= RunFrameHook::Install();
    Ok &= FrameStageNotifyHook::Install();
    Ok &= GcSendHook::Install();
    Ok &= GcRecvHook::Install();
    Ok &= EconWearableSpawnHook::Install();
    Ok &= GemRulesHook::Install();
    Ok &= ItemInLoadoutHook::Install();
    Ok &= NetFilterHook::Install();
    Ok &= SequenceDataChangedHook::Install();
    return Ok;
}
