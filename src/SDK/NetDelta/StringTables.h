#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class CSVCMsg_CreateStringTable;
class CSVCMsg_UpdateStringTable;

namespace NetDelta
{
    bool SnappyDecompress(const uint8_t* Data, size_t Size, std::vector<uint8_t>& Out, size_t Expected);

    class StringTables
    {
    public:
        void OnCreate(const CSVCMsg_CreateStringTable& Msg);
        void OnUpdate(const CSVCMsg_UpdateStringTable& Msg);
        void Clear();

        const std::vector<uint8_t>* Baseline(int ClassId) const;

    private:
        struct Table
        {
            std::string Name;
            bool        UserDataFixed = false;
            int32_t     UserDataSizeBits = 0;
            int32_t     Flags = 0;
            bool        VarintBitCounts = false;
            std::unordered_map<int, std::string> Keys;
        };

        void Parse(Table& T, const uint8_t* Data, size_t Size, int Count);

        std::vector<Table> Tables;
        std::unordered_map<int, std::vector<uint8_t>> Baselines;
    };
}
