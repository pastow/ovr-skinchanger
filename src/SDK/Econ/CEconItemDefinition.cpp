#include "CEconItemDefinition.h"

#include "Misc/Offsets.h"
#include "SDK/Memory/Memory.h"

#include <cstring>

namespace
{
    constexpr const char* TypeName = ".?AVCDOTAItemDefinition@@";

    std::uintptr_t Self(const void* Object)
    {
        return reinterpret_cast<std::uintptr_t>(Object);
    }
}

std::uint32_t CEconItemDefinition::DefIndex() const
{
    return Memory::Read<std::uint32_t>(Self(this) + Offsets::CEconItemDefinition::DefIndex);
}

std::uint32_t CEconItemDefinition::LoadoutSlot() const
{
    return Memory::Read<std::uint32_t>(Self(this) + Offsets::CEconItemDefinition::LoadoutSlot);
}

std::uint32_t CEconItemDefinition::HeroId() const
{
    return Memory::Read<std::uint32_t>(Self(this) + Offsets::CEconItemDefinition::HeroId);
}

bool CEconItemDefinition::IsBundle() const
{
    return Memory::Read<std::uintptr_t>(Self(this) + Offsets::CEconItemDefinition::BundleInfo) != 0;
}

std::vector<CEconItemDefinition*> CEconItemDefinition::BundleItems() const
{
    std::vector<CEconItemDefinition*> Out;
    std::uintptr_t Bundle = Memory::Read<std::uintptr_t>(Self(this) + Offsets::CEconItemDefinition::BundleInfo);
    if (!Bundle)
        return Out;

    int Count = Memory::Read<int>(Bundle + Offsets::BundleInfo::Count);
    auto* Items = Memory::Read<CEconItemDefinition**>(Bundle + Offsets::BundleInfo::Items);
    for (int Index = 0; Index < Count && Items; ++Index)
    {
        CEconItemDefinition* Item = Items[Index];
        const char* Name = Item ? Memory::RttiName(Item) : nullptr;
        if (Name && std::strcmp(Name, TypeName) == 0)
            Out.push_back(Item);
    }
    return Out;
}

std::vector<CEconItemDefinition::AssetModifier> CEconItemDefinition::AssetModifiers(int Style) const
{
    struct Entry
    {
        const char* Type;
        const char* Asset;
        const char* Modifier;
        std::uint8_t Rest[Offsets::AssetModifierEntry::Size - 3 * sizeof(const char*)];
    };
    struct Vector
    {
        std::int32_t Count;
        std::int32_t Pad;
        Entry* Data;
        std::int32_t Allocated;
        std::uint32_t Flags;
    };

    static thread_local Entry Buffer[Offsets::AssetModifierEntry::Capacity];
    Vector List {0, 0, Buffer, Offsets::AssetModifierEntry::Capacity, 0};

    using GetFn = void (*)(const CEconItemDefinition*, int, Vector*);
    auto** Vtable = Memory::Read<void**>(Self(this));
    reinterpret_cast<GetFn>(Vtable[Offsets::CEconItemDefinition::GetAssetModifiersIndex])(this, Style, &List);

    std::vector<AssetModifier> Out;
    for (int Index = 0; Index < List.Count && Index < Offsets::AssetModifierEntry::Capacity; ++Index)
    {
        const Entry& Item = List.Data[Index];
        Out.push_back({Item.Type ? Item.Type : "", Item.Asset ? Item.Asset : "", Item.Modifier ? Item.Modifier : ""});
    }
    return Out;
}

std::vector<std::string> CEconItemDefinition::Models() const
{
    std::vector<std::string> Out;
    std::uintptr_t Vector = Self(this) + Offsets::CEconItemDefinition::Models;
    int Count = Memory::Read<int>(Vector + Offsets::CUtlVector::Size);
    auto* Data = Memory::Read<const char* const*>(Vector + Offsets::CUtlVector::Data);
    for (int Index = 0; Index < Count && Data; ++Index)
        if (Data[Index] && *Data[Index])
            Out.emplace_back(Data[Index]);

    std::uintptr_t Visuals = Memory::Read<std::uintptr_t>(Self(this) + Offsets::CEconItemDefinition::Visuals);
    std::uintptr_t Styles = Visuals ? Memory::Read<std::uintptr_t>(Visuals + Offsets::ItemVisuals::Styles) : 0;
    int StyleCount = Styles ? Memory::Read<int>(Styles + Offsets::CUtlVector::Size) : 0;
    auto* StyleData = Styles ? Memory::Read<const std::uintptr_t*>(Styles + Offsets::CUtlVector::Data) : nullptr;
    for (int Index = 0; Index < StyleCount && StyleData; ++Index)
    {
        if (!StyleData[Index])
            continue;
        const char* Model = Memory::Read<const char*>(StyleData[Index] + Offsets::ItemStyle::Model);
        if (Model && *Model)
            Out.emplace_back(Model);
    }
    return Out;
}

int CEconItemDefinition::StyleSkin(int Style) const
{
    std::uintptr_t Visuals = Memory::Read<std::uintptr_t>(Self(this) + Offsets::CEconItemDefinition::Visuals);
    if (!Visuals)
        return 0;
    std::uintptr_t Styles = Memory::Read<std::uintptr_t>(Visuals + Offsets::ItemVisuals::Styles);
    int StyleCount = Styles ? Memory::Read<int>(Styles + Offsets::CUtlVector::Size) : 0;
    if (StyleCount <= 0)
        return Memory::Read<int>(Visuals + Offsets::ItemVisuals::Skin);
    auto* StyleData = Memory::Read<const std::uintptr_t*>(Styles + Offsets::CUtlVector::Data);
    if (Style < 0 || Style >= StyleCount || !StyleData || !StyleData[Style])
        return -1;
    return Memory::Read<int>(StyleData[Style] + Offsets::ItemStyle::Skin);
}
