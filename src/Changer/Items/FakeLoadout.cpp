#include "FakeLoadout.h"

#include "Changer/Inventory/FakeItems.h"
#include "SDK/Econ/CEconItemSchema.h"

namespace
{
    constexpr std::uint32_t StyleUnset = 0xFF;
}

std::vector<CEconItemDefinition::AssetModifier> FakeLoadout::EquippedModifiers()
{
    std::vector<CEconItemDefinition::AssetModifier> Out;
    CEconItemSchema* Schema = CEconItemSchema::Get();
    if (!Schema)
        return Out;

    for (const CSOEconItem& Item : FakeItems::All())
    {
        if (Item.equipped_state_size() == 0)
            continue;
        CEconItemDefinition* Def = Schema->GetItemDefinition(Item.def_index());
        if (!Def)
            continue;
        int Style = Item.style() == StyleUnset ? 0 : static_cast<int>(Item.style());
        for (auto& Modifier : Def->AssetModifiers(Style))
            Out.push_back(std::move(Modifier));
    }
    return Out;
}

std::vector<std::string> FakeLoadout::EquippedModels()
{
    std::vector<std::string> Out;
    CEconItemSchema* Schema = CEconItemSchema::Get();
    if (!Schema)
        return Out;

    for (const CSOEconItem& Item : FakeItems::All())
    {
        if (Item.equipped_state_size() == 0)
            continue;
        if (CEconItemDefinition* Def = Schema->GetItemDefinition(Item.def_index()))
            for (auto& Model : Def->Models())
                Out.push_back(std::move(Model));
    }
    return Out;
}

std::string FakeLoadout::Find(const std::vector<CEconItemDefinition::AssetModifier>& Modifiers, const char* Type, const char* Asset)
{
    for (const auto& Modifier : Modifiers)
        if (Modifier.Type == Type && Modifier.Asset == Asset && !Modifier.Modifier.empty())
            return Modifier.Modifier;
    return {};
}

FakeLoadout::Source FakeLoadout::FindSource(const char* Type, const char* Asset)
{
    Source Out;
    CEconItemSchema* Schema = CEconItemSchema::Get();
    if (!Schema)
        return Out;

    for (const CSOEconItem& Item : FakeItems::All())
    {
        if (Item.equipped_state_size() == 0)
            continue;
        CEconItemDefinition* Def = Schema->GetItemDefinition(Item.def_index());
        if (!Def)
            continue;
        int Style = Item.style() == StyleUnset ? 0 : static_cast<int>(Item.style());
        std::string Modifier = Find(Def->AssetModifiers(Style), Type, Asset);
        if (Modifier.empty())
            continue;
        Out.Modifier = std::move(Modifier);
        Out.Definition = Def;
        Out.Style = Style;
        return Out;
    }
    return Out;
}
