#pragma once
#include "BitStream.h"
#include <cstdint>
#include <string>
#include <vector>

namespace NetDelta
{
    struct FieldPath
    {
        int Path[7] = { -1, 0, 0, 0, 0, 0, 0 };
        int Last = 0;

        bool operator==(const FieldPath& Other) const;
        bool operator<(const FieldPath& Other) const;
        std::string Key() const;
    };

    bool DecodeFieldPaths(BitReader& Reader, std::vector<FieldPath>& Out);
    bool EncodeFieldPaths(BitWriter& Writer, const std::vector<FieldPath>& Paths);
}
