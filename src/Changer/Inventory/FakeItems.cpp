#include "FakeItems.h"

#include "SDK/Config/Config.h"
#include "SDK/Logger/Logger.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace
{
    constexpr const char* ConfigKey = "skin.items";
    constexpr const char* HexDigits = "0123456789abcdef";

    std::recursive_mutex                           ItemsMutex;
    std::unordered_map<std::uint64_t, CSOEconItem> Items;
    bool Loaded = false;

    int HexValue(char C)
    {
        if (C >= '0' && C <= '9')
            return C - '0';
        if (C >= 'a' && C <= 'f')
            return C - 'a' + 10;
        if (C >= 'A' && C <= 'F')
            return C - 'A' + 10;
        return -1;
    }

    void Load()
    {
        Loaded = true;
        std::string Value = Config::Get(ConfigKey);
        std::size_t Start = 0;
        while (Start < Value.size())
        {
            std::size_t End = Value.find(';', Start);
            if (End == std::string::npos)
                End = Value.size();

            std::string Bytes;
            bool Valid = (End - Start) % 2 == 0;
            for (std::size_t Index = Start; Valid && Index + 1 < End; Index += 2)
            {
                int High = HexValue(Value[Index]);
                int Low = HexValue(Value[Index + 1]);
                Valid = High >= 0 && Low >= 0;
                Bytes.push_back(static_cast<char>((High << 4) | Low));
            }

            CSOEconItem Item;
            if (Valid && Item.ParseFromString(Bytes) && Item.id())
                Items[Item.id()] = Item;
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
        for (const auto& [Id, Item] : Items)
        {
            for (unsigned char C : Item.SerializeAsString())
            {
                Value.push_back(HexDigits[C >> 4]);
                Value.push_back(HexDigits[C & 0xF]);
            }
            Value.push_back(';');
        }
        Config::Set(ConfigKey, Value);
    }
}

bool FakeItems::IsFake(std::uint64_t ItemId)
{
    return (ItemId >> 56) == (IdBase >> 56);
}

void FakeItems::Store(const CSOEconItem& Item)
{
    std::lock_guard<std::recursive_mutex> Lock(ItemsMutex);
    EnsureLoaded();
    Items[Item.id()] = Item;
    Save();
}

void FakeItems::Store(const std::vector<CSOEconItem>& Batch)
{
    std::lock_guard<std::recursive_mutex> Lock(ItemsMutex);
    EnsureLoaded();
    for (const CSOEconItem& Item : Batch)
        Items[Item.id()] = Item;
    Save();
}

std::optional<CSOEconItem> FakeItems::Find(std::uint64_t ItemId)
{
    std::lock_guard<std::recursive_mutex> Lock(ItemsMutex);
    EnsureLoaded();
    auto It = Items.find(ItemId);
    if (It == Items.end())
        return std::nullopt;
    return It->second;
}

std::vector<CSOEconItem> FakeItems::All()
{
    std::lock_guard<std::recursive_mutex> Lock(ItemsMutex);
    EnsureLoaded();
    std::vector<CSOEconItem> Out;
    Out.reserve(Items.size());
    for (const auto& [Id, Item] : Items)
        Out.push_back(Item);
    return Out;
}

void FakeItems::ForEach(const std::function<void(CSOEconItem&)>& Visit)
{
    std::lock_guard<std::recursive_mutex> Lock(ItemsMutex);
    EnsureLoaded();
    for (auto& [Id, Item] : Items)
        Visit(Item);
}
