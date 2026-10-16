#include "ItemSockets.h"

namespace
{
    constexpr std::uint32_t RequiredTypeEffect = 1;
    constexpr std::uint32_t RequiredTypeColor = 2;
    constexpr std::uint32_t RequiredTypeAny = 0;

    void Fill(CProtoItemSocket* Socket, std::uint32_t RequiredType, std::uint32_t GemDef)
    {
        Socket->set_required_hero("");
        Socket->set_required_item_slot("");
        Socket->set_required_type(RequiredType);
        Socket->set_gem_def_index(GemDef);
        Socket->set_not_tradable(false);
    }
}

bool ItemSockets::IsSocket(std::uint32_t AttributeDef)
{
    return AttributeDef >= FirstSocket && AttributeDef <= LastSocket;
}

std::optional<std::string> ItemSockets::Bytes(const CSOEconItem& Item, std::uint32_t AttributeDef)
{
    for (const auto& Attribute : Item.attribute())
        if (Attribute.def_index() == AttributeDef)
            return Attribute.value_bytes();
    return std::nullopt;
}

void ItemSockets::Set(CSOEconItem& Item, std::uint32_t AttributeDef, const std::string& Value)
{
    for (auto& Attribute : *Item.mutable_attribute())
    {
        if (Attribute.def_index() == AttributeDef)
        {
            Attribute.set_value_bytes(Value);
            return;
        }
    }
    auto* Attribute = Item.add_attribute();
    Attribute->set_def_index(AttributeDef);
    Attribute->set_value_bytes(Value);
}

bool ItemSockets::Erase(CSOEconItem& Item, std::uint32_t AttributeDef)
{
    auto* Attributes = Item.mutable_attribute();
    for (int Index = 0; Index < Attributes->size(); ++Index)
    {
        if (Attributes->Get(Index).def_index() == AttributeDef)
        {
            Attributes->DeleteSubrange(Index, 1);
            return true;
        }
    }
    return false;
}

std::optional<CProtoItemSocket> ItemSockets::Read(const std::string& Value)
{
    CProtoItemSocket_Empty Socket;
    if (!Socket.ParseFromString(Value))
        return std::nullopt;
    return Socket.socket();
}

std::string ItemSockets::EmptyBytes(std::uint32_t RequiredType)
{
    CProtoItemSocket_Empty Socket;
    Fill(Socket.mutable_socket(), RequiredType, 0);
    return Socket.SerializeAsString();
}

std::string ItemSockets::EffectBytes(std::uint32_t GemDef, std::uint32_t Effect)
{
    CProtoItemSocket_Effect Socket;
    Fill(Socket.mutable_socket(), RequiredTypeEffect, GemDef);
    Socket.set_effect(Effect);
    return Socket.SerializeAsString();
}

std::string ItemSockets::ColorBytes(std::uint8_t Red, std::uint8_t Green, std::uint8_t Blue)
{
    CProtoItemSocket_Color Socket;
    Fill(Socket.mutable_socket(), RequiredTypeColor, PrismaticGemDef);
    Socket.set_red(Red);
    Socket.set_green(Green);
    Socket.set_blue(Blue);
    return Socket.SerializeAsString();
}

std::uint32_t ItemSockets::FreeSocket(const CSOEconItem& Item)
{
    for (std::uint32_t Def = FirstSocket; Def <= LastSocket; ++Def)
        if (!Bytes(Item, Def))
            return Def;
    return 0;
}

std::uint32_t ItemSockets::FirstEmptySocket(const CSOEconItem& Item)
{
    for (std::uint32_t Def = FirstSocket; Def <= LastSocket; ++Def)
    {
        auto Value = Bytes(Item, Def);
        auto Socket = Value ? Read(*Value) : std::nullopt;
        if (Socket && Socket->gem_def_index() == 0)
            return Def;
    }
    return 0;
}

std::uint32_t ItemSockets::NextRequiredType(const CSOEconItem& Item)
{
    int Count = 0;
    for (std::uint32_t Def = FirstSocket; Def <= LastSocket; ++Def)
    {
        auto Value = Bytes(Item, Def);
        auto Socket = Value ? Read(*Value) : std::nullopt;
        if (Socket && Socket->gem_def_index() != UnusualGemDef)
            ++Count;
    }
    if (Count == 0)
        return RequiredTypeColor;
    if (Count == 1)
        return RequiredTypeEffect;
    return RequiredTypeAny;
}

std::uint32_t ItemSockets::FindGem(const CSOEconItem& Item, std::uint32_t GemDef)
{
    for (std::uint32_t Def = FirstSocket; Def <= LastSocket; ++Def)
    {
        auto Value = Bytes(Item, Def);
        auto Socket = Value ? Read(*Value) : std::nullopt;
        if (Socket && Socket->gem_def_index() == GemDef)
            return Def;
    }
    return 0;
}

void ItemSockets::SetUnusual(CSOEconItem& Item, int Effect, std::uint32_t PlainQuality)
{
    std::uint32_t Socket = FindGem(Item, UnusualGemDef);
    if (Effect <= 0)
    {
        if (Socket)
            Erase(Item, Socket);
        Item.set_quality(PlainQuality);
        return;
    }
    if (!Socket)
        Socket = FreeSocket(Item);
    if (!Socket)
        return;
    Set(Item, Socket, EffectBytes(UnusualGemDef, static_cast<std::uint32_t>(Effect)));
    Item.set_quality(QualityUnusual);
}
