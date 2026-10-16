#include "CDOTAInventoryManager.h"

#include "Misc/Offsets.h"
#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <cstdint>

namespace
{
    std::uintptr_t Manager = 0;
}

bool CDOTAInventoryManager::Init()
{
    std::uintptr_t Site = Memory::FindPattern("client.dll", Patterns::InventoryManager);
    if (!Site)
    {
        LogE("InventoryManager pattern not found");
        return false;
    }
    Manager = Memory::ResolveRelative(Site, 3, 7);
    Log("CDOTAInventoryManager %p", reinterpret_cast<void*>(Manager));
    return true;
}

CDOTAInventoryManager* CDOTAInventoryManager::Get()
{
    return reinterpret_cast<CDOTAInventoryManager*>(Manager);
}

CDOTAPlayerInventory* CDOTAInventoryManager::LocalInventory()
{
    auto** Vtable = Memory::Read<void**>(reinterpret_cast<std::uintptr_t>(this));
    return reinterpret_cast<CDOTAPlayerInventory* (*)(CDOTAInventoryManager*)>(Vtable[Offsets::CDOTAInventoryManager::LocalInventoryIndex])(this);
}
