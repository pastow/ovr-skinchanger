#pragma once

#include <cstdint>
#include <string>
#include <vector>

class CEconItemDefinition
{
public:
    struct AssetModifier
    {
        std::string Type;
        std::string Asset;
        std::string Modifier;
    };

    std::uint32_t DefIndex() const;
    std::uint32_t LoadoutSlot() const;
    std::uint32_t HeroId() const;
    bool IsBundle() const;
    std::vector<CEconItemDefinition*> BundleItems() const;
    std::vector<AssetModifier> AssetModifiers(int Style) const;
    std::vector<std::string> Models() const;
    int StyleSkin(int Style) const;
};
