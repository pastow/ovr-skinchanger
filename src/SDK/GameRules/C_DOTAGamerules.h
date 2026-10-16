#pragma once

#include <cstdint>

enum class EGameState : std::int32_t
{
    Init = 0,
    WaitForPlayersToLoad = 1,
    HeroSelection = 2,
    StrategyTime = 3,
    PreGame = 4,
    GameInProgress = 5,
    PostGame = 6,
    Disconnect = 7,
    TeamShowcase = 8,
    CustomGameSetup = 9,
    WaitForMapToLoad = 10,
    ScenarioSetup = 11,
    PlayerDraft = 12,
    Unknown = -1
};

class C_DOTAGamerules
{
public:
    static bool Init();
    static C_DOTAGamerules* Get();

    static EGameState CurrentState();
    static bool IsInGame();

    EGameState GameState() const;
};
