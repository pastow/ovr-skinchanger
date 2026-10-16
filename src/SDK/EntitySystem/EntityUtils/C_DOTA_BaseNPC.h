#pragma once

#include "SDK/EntitySystem/EntityUtils/C_BaseEntity.h"

#include <cstdint>
#include <cstdint>
#include <vector>

class C_DOTA_BaseNPC : public C_BaseEntity
{
public:
    static bool Init();

    std::vector<CHandle> MyWearables() const;
    std::int32_t NetworkActivity() const;
    std::int32_t NetworkSequenceIndex() const;
    void SetNetworkSequenceIndex(std::int32_t Variant);
    std::int32_t SelectWeightedSequence(std::int32_t Activity);

};
