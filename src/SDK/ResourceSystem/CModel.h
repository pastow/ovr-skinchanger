#pragma once

#include <cstdint>

class CModel
{
public:
    static bool Init();

    const char* SequenceName(int Index) const;
    int LookupSequence(const char* Name) const;
    int PoseParameterCount() const;
    const char* PoseParameterName(int Index) const;
    int LookupPoseParameter(const char* Name) const;
    int SequenceForVariant(int Activity, int Variant);
    int VariantOf(int Activity, int Sequence);
    int VariantNamed(int Activity, const char* Part);
    std::uint32_t MaterialGroup(int Skin) const;
};
