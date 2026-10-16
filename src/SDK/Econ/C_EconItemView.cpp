#include "C_EconItemView.h"

#include "Misc/Offsets.h"
#include "Misc/Patterns.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

namespace
{
    constexpr std::uint32_t DefaultQuality = 9999;
    constexpr std::uint32_t DefaultLevel = 9999;

    C_EconItemView* (*CtorFn)(C_EconItemView*) = nullptr;
    void (*InitFn)(C_EconItemView*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, void*, std::uint8_t) = nullptr;
    void (*DtorFn)(C_EconItemView*) = nullptr;
}

bool C_EconItemView::Init()
{
    CtorFn = reinterpret_cast<decltype(CtorFn)>(Memory::FindPattern("client.dll", Patterns::EconItemViewCtor));
    InitFn = reinterpret_cast<decltype(InitFn)>(Memory::FindPattern("client.dll", Patterns::EconItemViewInit));
    DtorFn = reinterpret_cast<decltype(DtorFn)>(Memory::FindPattern("client.dll", Patterns::EconItemViewDtor));
    if (!CtorFn || !InitFn || !DtorFn)
    {
        LogE("C_EconItemView patterns not found ctor %p init %p dtor %p", reinterpret_cast<void*>(CtorFn), reinterpret_cast<void*>(InitFn),
             reinterpret_cast<void*>(DtorFn));
        return false;
    }
    return true;
}

void C_EconItemView::Construct(C_EconItemView* View)
{
    CtorFn(View);
}

void C_EconItemView::Initialize(std::uint32_t DefIndex, std::uint32_t AccountId, std::uint8_t Style)
{
    InitFn(this, DefIndex, DefaultQuality, DefaultLevel, AccountId, nullptr, Style);
}

void C_EconItemView::Destruct()
{
    DtorFn(this);
}

std::uint16_t C_EconItemView::ItemDefinitionIndex() const
{
    return Memory::Read<std::uint16_t>(reinterpret_cast<std::uintptr_t>(this) + Offsets::C_EconItemView::ItemDefinitionIndex);
}

std::uint64_t C_EconItemView::ItemId() const
{
    return Memory::Read<std::uint64_t>(reinterpret_cast<std::uintptr_t>(this) + Offsets::C_EconItemView::ItemId);
}

std::uint32_t C_EconItemView::AccountId() const
{
    return Memory::Read<std::uint32_t>(reinterpret_cast<std::uintptr_t>(this) + Offsets::C_EconItemView::AccountId);
}

std::uint8_t C_EconItemView::Style() const
{
    return Memory::Read<std::uint8_t>(reinterpret_cast<std::uintptr_t>(this) + Offsets::C_EconItemView::Style);
}

void C_EconItemView::SetItemId(std::uint64_t ItemId)
{
    *reinterpret_cast<std::uint64_t*>(reinterpret_cast<std::uintptr_t>(this) + Offsets::C_EconItemView::ItemId) = ItemId;
}
