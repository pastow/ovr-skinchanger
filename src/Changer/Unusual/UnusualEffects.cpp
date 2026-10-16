#include "UnusualEffects.h"

namespace
{
    constexpr UnusualEffects::Effect Effects[] = {
        { "None", 0 },
        { "Affliction of Vermin", 22 },
        { "Bleak Hallucination", 159 },
        { "Burning Animus", 19 },
        { "Butterfly Romp", 196 },
        { "Candy Caster", 827 },
        { "Champion's Aura 2012", 37 },
        { "Champion's Aura 2013", 109 },
        { "Champion's Aura 2014", 268 },
        { "Crystal Rift", 68 },
        { "Cursed Essence", 73 },
        { "Diretide Blight", 155 },
        { "Diretide Corruption", 46 },
        { "Divine Essence", 74 },
        { "Emerald Ectoplasm", 138 },
        { "Ethereal Flame", 15 },
        { "Featherfall", 836 },
        { "Felicity's Blessing", 21 },
        { "Frostbloom", 826 },
        { "Frostivus Frost", 57 },
        { "Gold Drop", 833 },
        { "Gold Glow", 834 },
        { "Ionic Vapor", 163 },
        { "Luminous Gaze", 17 },
        { "New Bloom Celebration", 185 },
        { "Orbital Decay", 158 },
        { "Piercing Beams", 20 },
        { "Ravenblight", 837 },
        { "Resonant Energy", 16 },
        { "Rubiline Sheen", 129 },
        { "Searing Essence", 18 },
        { "Self-Made Aura", 4 },
        { "Smoke", 835 },
        { "Snowdrift", 828 },
        { "Spirit of Earth", 157 },
        { "Spirit of Ember", 156 },
        { "Sunfire", 32 },
        { "Touch of Flame", 206 },
        { "Touch of Frost", 205 },
        { "Touch of Midas", 47 },
        { "Trail of Burning Doom", 96 },
        { "Trail of the Amanita", 76 },
        { "Trail of the Lotus Blossom", 61 },
        { "Triumph of Champions", 31 },
    };
}

std::span<const UnusualEffects::Effect> UnusualEffects::All()
{
    return Effects;
}

const UnusualEffects::Effect* UnusualEffects::Find(int Id)
{
    for (const Effect& Entry : Effects)
    {
        if (Entry.Id == Id)
            return &Entry;
    }
    return nullptr;
}
