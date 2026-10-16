#include "RealItems.h"

#include <mutex>
#include <unordered_map>

namespace
{
    std::mutex                                     ItemsMutex;
    std::unordered_map<std::uint64_t, CSOEconItem> Items;
}

void RealItems::Clear()
{
    std::lock_guard<std::mutex> Lock(ItemsMutex);
    Items.clear();
}

void RealItems::Set(const CSOEconItem& Item)
{
    std::lock_guard<std::mutex> Lock(ItemsMutex);
    Items[Item.id()] = Item;
}

void RealItems::Remove(std::uint64_t ItemId)
{
    std::lock_guard<std::mutex> Lock(ItemsMutex);
    Items.erase(ItemId);
}

void RealItems::ForEach(const std::function<void(const CSOEconItem&)>& Visit)
{
    std::lock_guard<std::mutex> Lock(ItemsMutex);
    for (const auto& [Id, Item] : Items)
        Visit(Item);
}
