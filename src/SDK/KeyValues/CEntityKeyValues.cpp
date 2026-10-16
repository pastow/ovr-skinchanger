#include "CEntityKeyValues.h"

#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

namespace
{
    struct EntityKeyId
    {
        std::uint32_t Hash;
        std::uint32_t Index;
        const char* Name;
    };

    KeyValues3* (*FindValueFn)(CEntityKeyValues*, const EntityKeyId*, void*) = nullptr;
}

bool CEntityKeyValues::Init()
{
    FindValueFn = reinterpret_cast<decltype(FindValueFn)>(Memory::FindPattern("client.dll", Patterns::EntityKeyValuesFindValue));
    if (!FindValueFn)
    {
        LogE("EntityKeyValuesFindValue pattern not found");
        return false;
    }
    return true;
}

KeyValues3* CEntityKeyValues::FindValue(std::uint32_t Hash, const char* Name)
{
    EntityKeyId Key {Hash, 0xFFFFFFFFu, Name};
    return FindValueFn(this, &Key, nullptr);
}
