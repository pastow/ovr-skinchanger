#include "CModel.h"

#include "Misc/Offsets.h"
#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <cstring>

namespace
{
    const char* (*SequenceNameFn)(const CModel*, int) = nullptr;
    int* (*LookupSequenceFn)(const CModel*, int*, const char*) = nullptr;
    int (*PoseParameterCountFn)(const CModel*) = nullptr;
    const char* (*PoseParameterNameFn)(const CModel*, int) = nullptr;
    int (*LookupPoseParameterFn)(const CModel*, const char*) = nullptr;
    void* (*ActivityTableFn)(CModel*) = nullptr;
    int* (*ActivitySequenceFn)(void*, int*, int, int) = nullptr;

    constexpr int MaxVariants = 64;

    template <typename T>
    bool Resolve(T& Target, const char* Pattern, const char* Name)
    {
        Target = reinterpret_cast<T>(Memory::FindPattern("client.dll", Pattern));
        if (!Target)
            LogE("CModel %s pattern not found", Name);
        return Target != nullptr;
    }
}

bool CModel::Init()
{
    bool Ok = true;
    Ok &= Resolve(SequenceNameFn, Patterns::ModelSequenceName, "SequenceName");
    Ok &= Resolve(LookupSequenceFn, Patterns::ModelLookupSequence, "LookupSequence");
    Ok &= Resolve(PoseParameterCountFn, Patterns::ModelPoseParameterCount, "PoseParameterCount");
    Ok &= Resolve(PoseParameterNameFn, Patterns::ModelPoseParameterName, "PoseParameterName");
    Ok &= Resolve(LookupPoseParameterFn, Patterns::ModelLookupPoseParameter, "LookupPoseParameter");
    Ok &= Resolve(ActivityTableFn, Patterns::ModelActivityTable, "ActivityTable");
    Ok &= Resolve(ActivitySequenceFn, Patterns::ModelActivitySequence, "ActivitySequence");
    return Ok;
}

const char* CModel::SequenceName(int Index) const
{
    return Index >= 0 ? SequenceNameFn(this, Index) : nullptr;
}

int CModel::LookupSequence(const char* Name) const
{
    if (!Name || !*Name)
        return -1;
    int Result = -1;
    LookupSequenceFn(this, &Result, Name);
    return Result;
}

int CModel::PoseParameterCount() const
{
    return PoseParameterCountFn(this);
}

const char* CModel::PoseParameterName(int Index) const
{
    if (Index < 0 || Index >= PoseParameterCount())
        return nullptr;
    return PoseParameterNameFn(this, Index);
}

int CModel::LookupPoseParameter(const char* Name) const
{
    if (!Name || !*Name)
        return -1;
    return LookupPoseParameterFn(this, Name);
}

int CModel::SequenceForVariant(int Activity, int Variant)
{
    void* Table = ActivityTableFn(this);
    if (!Table)
        return -1;
    int Sequence = -1;
    ActivitySequenceFn(Table, &Sequence, Variant, Activity);
    return Sequence;
}

int CModel::VariantOf(int Activity, int Sequence)
{
    int Previous = -1;
    for (int Variant = 0; Variant < MaxVariants; ++Variant)
    {
        int Current = SequenceForVariant(Activity, Variant);
        if (Current == Sequence)
            return Variant;
        if (Variant > 0 && Current == Previous)
            return -1;
        Previous = Current;
    }
    return -1;
}

std::uint32_t CModel::MaterialGroup(int Skin) const
{
    std::uintptr_t Self = reinterpret_cast<std::uintptr_t>(this);
    int Count = Memory::Read<int>(Self + Offsets::CModel::MaterialGroupCount);
    auto* Groups = Memory::Read<const std::uint32_t*>(Self + Offsets::CModel::MaterialGroups);
    if (Skin < 0 || Skin >= Count || !Groups)
        return 0;
    return Groups[Skin];
}

int CModel::VariantNamed(int Activity, const char* Part)
{
    int Previous = -1;
    for (int Variant = 0; Variant < MaxVariants; ++Variant)
    {
        int Current = SequenceForVariant(Activity, Variant);
        if (Current < 0 || (Variant > 0 && Current == Previous))
            return -1;
        const char* Name = SequenceName(Current);
        if (Name && std::strstr(Name, Part))
            return Variant;
        Previous = Current;
    }
    return -1;
}
