#pragma once

class CDOTAPlayerInventory;

class CDOTAInventoryManager
{
public:
    static bool Init();
    static CDOTAInventoryManager* Get();

    CDOTAPlayerInventory* LocalInventory();
};
