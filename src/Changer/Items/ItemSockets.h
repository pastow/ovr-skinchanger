#pragma once

#include "base_gcmessages.pb.h"
#include "econ_gcmessages.pb.h"

#include <cstdint>
#include <optional>
#include <string>

namespace ItemSockets
{
    constexpr std::uint32_t EtherealGemDef = 3000;
    constexpr std::uint32_t PrismaticGemDef = 3001;
    constexpr std::uint32_t UnusualGemDef = 3045;
    constexpr std::uint32_t FirstSocket = 1;
    constexpr std::uint32_t LastSocket = 8;
    constexpr std::uint32_t GemValueSocket = 1;
    constexpr std::uint32_t QualityUnusual = 3;
    constexpr std::uint32_t QualityUnique = 4;

    bool IsSocket(std::uint32_t AttributeDef);
    std::optional<std::string> Bytes(const CSOEconItem& Item, std::uint32_t AttributeDef);
    void Set(CSOEconItem& Item, std::uint32_t AttributeDef, const std::string& Bytes);
    bool Erase(CSOEconItem& Item, std::uint32_t AttributeDef);
    std::optional<CProtoItemSocket> Read(const std::string& Bytes);

    std::string EmptyBytes(std::uint32_t RequiredType);
    std::string EffectBytes(std::uint32_t GemDef, std::uint32_t Effect);
    std::string ColorBytes(std::uint8_t Red, std::uint8_t Green, std::uint8_t Blue);

    std::uint32_t FreeSocket(const CSOEconItem& Item);
    std::uint32_t FirstEmptySocket(const CSOEconItem& Item);
    std::uint32_t NextRequiredType(const CSOEconItem& Item);
    std::uint32_t FindGem(const CSOEconItem& Item, std::uint32_t GemDef);

    void SetUnusual(CSOEconItem& Item, int Effect, std::uint32_t PlainQuality);
}
