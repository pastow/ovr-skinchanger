#include "GemCatalog.h"

#include "Changer/Inventory/FakeItems.h"
#include "Changer/Items/ItemSockets.h"
#include "Changer/Unusual/UnusualEffects.h"
#include "SDK/Logger/Logger.h"
#include "SDK/SteamAPI/ISteamUser.h"

#include <cstdint>
#include <vector>

namespace
{
    constexpr std::uint32_t ChiselDef = 17003;
    constexpr std::uint32_t MasterHammerDef = 15339;

    struct Color
    {
        const char* Name;
        std::uint8_t Red;
        std::uint8_t Green;
        std::uint8_t Blue;
    };

    constexpr Color Colors[] = {
        { "Red", 0xD0, 0x3D, 0x33 },
        { "Rubiline", 0xD1, 0x1F, 0xA1 },
        { "Gold", 0xCF, 0xAB, 0x31 },
        { "Blue", 0x00, 0x97, 0xCE },
        { "Purple", 0x82, 0x32, 0xCF },
        { "Orange", 0xD0, 0x77, 0x33 },
        { "Light Green", 0xB7, 0xCF, 0x33 },
        { "Deep Blue", 0x3D, 0x68, 0xC4 },
        { "Sea Green", 0x4A, 0xB7, 0x8D },
        { "Verdant Green", 0x51, 0xB3, 0x50 },
        { "Deep Green", 0x37, 0x86, 0x4D },
        { "Bright Purple", 0x82, 0x32, 0xED },
        { "Bright Green", 0xA1, 0xFF, 0x59 },
        { "Placid Blue", 0x94, 0xCA, 0xD0 },
        { "Summer Warmth", 0xFF, 0xEE, 0xBC },
        { "Defensive Red", 0xFF, 0x42, 0x00 },
        { "Creator's Light", 0xDC, 0xF2, 0xFF },
        { "Blossom Red", 0xD7, 0x60, 0x92 },
        { "Crystalline Blue", 0x1A, 0x3D, 0x85 },
        { "Cursed Black", 0x06, 0x06, 0x06 },
        { "Plague Grey", 0x62, 0x6E, 0x5B },
        { "Champion's Blue", 0x50, 0x7D, 0xFE },
        { "Champion's Green", 0x15, 0xA5, 0x15 },
        { "Midas Gold", 0xFF, 0xCA, 0x15 },
        { "Earth Green", 0x5A, 0xC3, 0x55 },
        { "Ember Flame", 0xFF, 0xC6, 0x04 },
        { "Diretide Orange", 0xF7, 0x9D, 0x00 },
        { "Dredge Earth", 0xBD, 0xB7, 0x6B },
        { "Dungeon Doom", 0x7B, 0x68, 0xEE },
        { "Tnim S'nnam", 0xBC, 0xDD, 0xB3 },
        { "Brusque Britches Beige", 0xF0, 0xE6, 0x8C },
        { "Unhallowed Ground", 0x80, 0x80, 0x00 },
        { "Ships in the Night", 0x19, 0x19, 0x70 },
        { "Miasmatic Grey", 0xC0, 0xC0, 0xC0 },
        { "Pristine Platinum", 0xD5, 0xE3, 0xF5 },
        { "Vermillion Renewal", 0xCA, 0x01, 0x23 },
        { "Reflection's Shade", 0xFF, 0x3C, 0x28 },
        { "Pyroclastic Flow", 0xFF, 0x78, 0x32 },
        { "Glacial Flow", 0x32, 0xAB, 0xDC },
        { "Plushy Shag", 0xFF, 0xC1, 0xDC },
        { "Champion's Purple", 0x7F, 0x48, 0xC3 },
        { "Explosive Burst", 0xFF, 0xAF, 0x00 },
    };

    CSOEconItem Make(std::uint32_t Def, std::uint32_t Variant, std::uint32_t AccountId)
    {
        CSOEconItem Item;
        Item.set_id(FakeItems::IdBase | (static_cast<std::uint64_t>(Variant) << 32) | Def);
        Item.set_account_id(AccountId);
        Item.set_def_index(Def);
        Item.set_inventory(0);
        Item.set_quantity(1);
        Item.set_level(1);
        Item.set_quality(ItemSockets::QualityUnique);
        return Item;
    }
}

void GemCatalog::EnsureGranted()
{
    ISteamUser* User = ISteamUser::Get();
    std::uint32_t AccountId = User ? User->AccountId() : 0;

    std::vector<CSOEconItem> Missing;
    auto Want = [&](CSOEconItem Item) {
        if (!FakeItems::Find(Item.id()))
            Missing.push_back(std::move(Item));
    };

    Want(Make(ChiselDef, 0, AccountId));
    Want(Make(MasterHammerDef, 0, AccountId));

    for (const UnusualEffects::Effect& Entry : UnusualEffects::All())
    {
        if (Entry.Id <= 0)
            continue;
        CSOEconItem Gem = Make(ItemSockets::EtherealGemDef, static_cast<std::uint32_t>(Entry.Id), AccountId);
        ItemSockets::Set(Gem, ItemSockets::GemValueSocket, ItemSockets::EffectBytes(ItemSockets::EtherealGemDef, static_cast<std::uint32_t>(Entry.Id)));
        Want(std::move(Gem));
    }

    std::uint32_t Variant = 1;
    for (const Color& Entry : Colors)
    {
        CSOEconItem Gem = Make(ItemSockets::PrismaticGemDef, Variant++, AccountId);
        ItemSockets::Set(Gem, ItemSockets::GemValueSocket, ItemSockets::ColorBytes(Entry.Red, Entry.Green, Entry.Blue));
        Want(std::move(Gem));
    }

    if (Missing.empty())
        return;
    FakeItems::Store(Missing);
}
