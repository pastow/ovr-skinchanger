#include "C_DOTAGamerules.h"

#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"
#include "SDK/SchemaSystem/SchemaOffset.h"

namespace
{
    std::uintptr_t GameRulesPtr = 0;
    std::uint32_t GameStateOffset = 0;
}

bool C_DOTAGamerules::Init()
{
    std::uintptr_t Site = Memory::FindPattern("client.dll", Patterns::GameRules);
    if (!Site)
    {
        LogE("GameRules pattern not found");
        return false;
    }
    GameRulesPtr = Memory::ResolveRelative(Site, 3, 7);

    GameStateOffset = SchemaOffset::Instance()->GetOffset("C_DOTAGamerules", "m_nGameState");
    if (!GameStateOffset)
    {
        LogE("C_DOTAGamerules::m_nGameState not found in schema");
        return false;
    }

    Log("g_pGameRules %p m_nGameState 0x%X", reinterpret_cast<void*>(GameRulesPtr), GameStateOffset);
    return true;
}

C_DOTAGamerules* C_DOTAGamerules::Get()
{
    return GameRulesPtr ? Memory::Read<C_DOTAGamerules*>(GameRulesPtr) : nullptr;
}

EGameState C_DOTAGamerules::GameState() const
{
    return static_cast<EGameState>(Memory::Read<std::int32_t>(reinterpret_cast<std::uintptr_t>(this) + GameStateOffset));
}

EGameState C_DOTAGamerules::CurrentState()
{
    C_DOTAGamerules* Rules = Get();
    return Rules ? Rules->GameState() : EGameState::Unknown;
}

bool C_DOTAGamerules::IsInGame()
{
    EGameState State = CurrentState();
    return State == EGameState::PreGame || State == EGameState::GameInProgress;
}
