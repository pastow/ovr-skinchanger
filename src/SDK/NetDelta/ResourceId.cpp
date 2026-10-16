#include "ResourceId.h"
#include <cctype>
#include <cstring>
#include <string>

namespace NetDelta
{
    namespace
    {
        uint64_t MurmurHash64B(const uint8_t* Data, size_t Len, uint64_t Seed)
        {
            constexpr uint32_t M = 0x5bd1e995;
            constexpr int R = 24;
            uint32_t H1 = static_cast<uint32_t>(Seed) ^ static_cast<uint32_t>(Len);
            uint32_t H2 = static_cast<uint32_t>(Seed >> 32);

            auto Read32 = [](const uint8_t* P)
            {
                uint32_t V;
                std::memcpy(&V, P, 4);
                return V;
            };

            while (Len >= 8)
            {
                uint32_t K1 = Read32(Data);
                K1 *= M; K1 ^= K1 >> R; K1 *= M;
                H1 *= M; H1 ^= K1;
                uint32_t K2 = Read32(Data + 4);
                K2 *= M; K2 ^= K2 >> R; K2 *= M;
                H2 *= M; H2 ^= K2;
                Data += 8;
                Len -= 8;
            }
            if (Len >= 4)
            {
                uint32_t K1 = Read32(Data);
                K1 *= M; K1 ^= K1 >> R; K1 *= M;
                H1 *= M; H1 ^= K1;
                Data += 4;
                Len -= 4;
            }
            switch (Len)
            {
            case 3: H2 ^= static_cast<uint32_t>(Data[2]) << 16; [[fallthrough]];
            case 2: H2 ^= static_cast<uint32_t>(Data[1]) << 8; [[fallthrough]];
            case 1: H2 ^= Data[0]; H2 *= M;
            }
            H1 ^= H2 >> 18; H1 *= M;
            H2 ^= H1 >> 22; H2 *= M;
            H1 ^= H2 >> 17; H1 *= M;
            H2 ^= H1 >> 19; H2 *= M;
            return (static_cast<uint64_t>(H1) << 32) | H2;
        }
    }

    uint64_t ResourceId(const char* Path)
    {
        if (!Path || !*Path)
            return 0;
        std::string Lower(Path);
        for (char& C : Lower)
        {
            C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
            if (C == '\\')
                C = '/';
        }
        return MurmurHash64B(reinterpret_cast<const uint8_t*>(Lower.data()), Lower.size(), 0xEDABCDEF);
    }
}
