#include "SDK.h"


#include "SDK/Config/Config.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Panorama/UIEngine.h"
#include "SDK/SchemaSystem/CSchemaSystem.h"
#include "SDK/SchemaSystem/SchemaOffset.h"
#include "SDK/GC/CGCClient.h"
#include "SDK/Econ/CEconItemSchema.h"
#include "SDK/Econ/C_EconItemView.h"
#include "SDK/Econ/CDOTAInventoryManager.h"
#include "SDK/Econ/CDOTAPlayerInventory.h"
#include "SDK/KeyValues/CEntityKeyValues.h"
#include "SDK/ResourceSystem/CModel.h"
#include "SDK/ResourceSystem/CResourceSystem.h"
#include "SDK/Network/CNetworkGameClient.h"
#include "SDK/GameRules/C_DOTAGamerules.h"
#include "SDK/EntitySystem/CGameEntitySystem.h"
#include "SDK/EntitySystem/EntityUtils/C_BaseEntity.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTA_BaseNPC.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTA_BaseNPC_Hero.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTAPlayerController.h"
#include "SDK/SteamAPI/ISteamUser.h"
#include "SDK/V8/V8.h"

bool SDK::Init()
{
    if (!Config::Init())
    {
        LogE("Config init failed");
        return false;
    }

    if (!UIEngine::Init())
    {
        LogE("UIEngine init failed");
        return false;
    }

    if (!V8::Init())
    {
        LogE("V8 init failed");
        return false;
    }

    if (!CSchemaSystem::Init() || !SchemaOffset::Instance()->Init())
    {
        LogE("SchemaSystem init failed");
        return false;
    }

    if (!C_DOTAGamerules::Init())
    {
        LogE("C_DOTAGamerules init failed");
        return false;
    }

    if (!CGameEntitySystem::Init() || !C_BaseEntity::Init() || !C_DOTA_BaseNPC::Init() || !C_DOTA_BaseNPC_Hero::Init() ||
        !C_DOTAPlayerController::Init())
    {
        LogE("EntitySystem init failed");
        return false;
    }

    if (!CEconItemSchema::Init() || !C_EconItemView::Init() || !CEntityKeyValues::Init() || !CDOTAInventoryManager::Init() ||
        !CDOTAPlayerInventory::Init())
    {
        LogE("Econ init failed");
        return false;
    }

    if (!CResourceSystem::Init() || !CModel::Init())
    {
        LogE("ResourceSystem init failed");
        return false;
    }

    if (!CNetworkGameClient::Init())
    {
        LogE("NetworkGameClient init failed");
        return false;
    }

    if (!CGCClient::Init())
    {
        LogE("CGCClient init failed");
        return false;
    }

    if (!ISteamUser::Init())
    {
        LogE("ISteamUser init failed");
        return false;
    }

    return true;
}
