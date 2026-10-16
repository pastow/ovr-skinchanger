#include "CEconItemSchema.h"

#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

namespace
{
    CEconItemSchema* (*GetItemSchemaFn)() = nullptr;
    CEconItemDefinition* (*GetItemDefinitionFn)(CEconItemSchema*, std::uint32_t) = nullptr;
}

bool CEconItemSchema::Init()
{
    GetItemSchemaFn = reinterpret_cast<decltype(GetItemSchemaFn)>(Memory::FindPattern("client.dll", Patterns::GetItemSchema));
    GetItemDefinitionFn = reinterpret_cast<decltype(GetItemDefinitionFn)>(Memory::FindPattern("client.dll", Patterns::GetItemDefinition));
    if (!GetItemSchemaFn || !GetItemDefinitionFn)
    {
        LogE("ItemSchema patterns not found GetItemSchema %p GetItemDefinition %p", reinterpret_cast<void*>(GetItemSchemaFn),
             reinterpret_cast<void*>(GetItemDefinitionFn));
        return false;
    }
    Log("GetItemSchema %p GetItemDefinition %p", reinterpret_cast<void*>(GetItemSchemaFn), reinterpret_cast<void*>(GetItemDefinitionFn));
    return true;
}

CEconItemSchema* CEconItemSchema::Get()
{
    return GetItemSchemaFn ? GetItemSchemaFn() : nullptr;
}

CEconItemDefinition* CEconItemSchema::GetItemDefinition(std::uint32_t DefIndex)
{
    return GetItemDefinitionFn(this, DefIndex);
}
