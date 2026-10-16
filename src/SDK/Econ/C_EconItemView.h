#pragma once

#include <cstdint>

class C_EconItemView
{
public:
    static bool Init();
    static void Construct(C_EconItemView* View);

    void Initialize(std::uint32_t DefIndex, std::uint32_t AccountId, std::uint8_t Style);
    void Destruct();

    std::uint16_t ItemDefinitionIndex() const;
    std::uint64_t ItemId() const;
    std::uint32_t AccountId() const;
    std::uint8_t Style() const;

    void SetItemId(std::uint64_t ItemId);
};
