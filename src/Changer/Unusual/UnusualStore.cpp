#include "UnusualStore.h"

#include "SDK/Config/Config.h"
#include "SDK/Logger/Logger.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{
    constexpr const char* ConfigKey = "skin.unusual";

    std::map<std::uint64_t, int> Effects;
    std::vector<UnusualStore::Listener> Listeners;
    bool Loaded = false;

    void Load()
    {
        Loaded = true;
        Effects.clear();

        std::string Value = Config::Get(ConfigKey);
        std::size_t Start = 0;
        while (Start < Value.size())
        {
            std::size_t End = Value.find(';', Start);
            if (End == std::string::npos)
                End = Value.size();

            std::string Entry = Value.substr(Start, End - Start);
            std::size_t Separator = Entry.find(':');
            if (Separator != std::string::npos)
            {
                std::uint64_t ItemId = std::strtoull(Entry.c_str(), nullptr, 16);
                int EffectId = std::atoi(Entry.c_str() + Separator + 1);
                if (ItemId && EffectId)
                    Effects[ItemId] = EffectId;
            }
            Start = End + 1;
        }

    }

    void EnsureLoaded()
    {
        if (!Loaded)
            Load();
    }

    void Save()
    {
        std::string Value;
        char Entry[48];
        for (const auto& [ItemId, EffectId] : Effects)
        {
            std::snprintf(Entry, sizeof(Entry), "%" PRIx64 ":%d;", ItemId, EffectId);
            Value += Entry;
        }
        Config::Set(ConfigKey, Value);
    }
}

int UnusualStore::Get(std::uint64_t ItemId)
{
    EnsureLoaded();
    auto Entry = Effects.find(ItemId);
    return Entry != Effects.end() ? Entry->second : 0;
}

void UnusualStore::Set(std::uint64_t ItemId, int EffectId)
{
    EnsureLoaded();
    if (!ItemId || Get(ItemId) == EffectId)
        return;

    if (EffectId)
        Effects[ItemId] = EffectId;
    else
        Effects.erase(ItemId);
    Save();

    for (Listener Callback : Listeners)
        Callback(ItemId, EffectId);
}

const std::map<std::uint64_t, int>& UnusualStore::All()
{
    EnsureLoaded();
    return Effects;
}

void UnusualStore::Subscribe(Listener Callback)
{
    if (Callback)
        Listeners.push_back(Callback);
}
