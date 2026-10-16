#pragma once

#include "SDK/Econ/CEconItemDefinition.h"

#include <string>
#include <vector>

namespace FakeLoadout
{
    std::vector<CEconItemDefinition::AssetModifier> EquippedModifiers();
    std::vector<std::string> EquippedModels();
    std::string Find(const std::vector<CEconItemDefinition::AssetModifier>& Modifiers, const char* Type, const char* Asset);

    struct Source
    {
        std::string Modifier;
        CEconItemDefinition* Definition = nullptr;
        int Style = 0;
    };
    Source FindSource(const char* Type, const char* Asset);
}
