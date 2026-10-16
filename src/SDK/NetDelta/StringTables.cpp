#include "StringTables.h"
#include "BitStream.h"
#include "SDK/Logger/Logger.h"
#include "netmessages.pb.h"
#include <cstdlib>
#include <cstring>

namespace NetDelta
{
    bool SnappyDecompress(const uint8_t* Data, size_t Size, std::vector<uint8_t>& Out, size_t Expected)
    {
        size_t   Offset = 0;
        uint32_t Length = 0;
        if (!ReadVarUint32Bytes(Data, Size, Offset, Length))
            return false;

        if (Length != Expected)
            return false;
        Out.clear();
        while (Offset < Size)
        {
            uint8_t Tag  = Data[Offset++];
            uint32_t Type = Tag & 3;
            if (Type == 0)
            {
                uint32_t Len = (Tag >> 2) + 1;
                if (Len > 60)
                {
                    uint32_t Bytes = Len - 60;
                    if (Offset + Bytes > Size)
                        return false;
                    Len = 0;
                    for (uint32_t I = 0; I < Bytes; ++I)
                        Len |= static_cast<uint32_t>(Data[Offset + I]) << (8 * I);
                    Len += 1;
                    Offset += Bytes;
                }
                if (Offset + Len > Size || Out.size() + Len > Length)
                    return false;
                Out.insert(Out.end(), Data + Offset, Data + Offset + Len);
                Offset += Len;
                continue;
            }

            uint32_t Len = 0;
            uint32_t Dist = 0;
            if (Type == 1)
            {
                if (Offset >= Size)
                    return false;
                Len  = ((Tag >> 2) & 7) + 4;
                Dist = (static_cast<uint32_t>(Tag >> 5) << 8) | Data[Offset++];
            }
            else if (Type == 2)
            {
                if (Offset + 2 > Size)
                    return false;
                Len  = (Tag >> 2) + 1;
                Dist = Data[Offset] | (Data[Offset + 1] << 8);
                Offset += 2;
            }
            else
            {
                if (Offset + 4 > Size)
                    return false;
                Len  = (Tag >> 2) + 1;
                Dist = Data[Offset] | (Data[Offset + 1] << 8) | (Data[Offset + 2] << 16) |
                       (static_cast<uint32_t>(Data[Offset + 3]) << 24);
                Offset += 4;
            }

            if (Dist == 0 || Dist > Out.size() || Out.size() + Len > Length)
                return false;
            size_t From = Out.size() - Dist;
            for (uint32_t I = 0; I < Len; ++I)
                Out.push_back(Out[From + I]);
        }
        return Out.size() == Length;
    }

    void StringTables::Clear()
    {
        Tables.clear();
        Baselines.clear();
    }

    const std::vector<uint8_t>* StringTables::Baseline(int ClassId) const
    {
        auto It = Baselines.find(ClassId);
        return It != Baselines.end() ? &It->second : nullptr;
    }

    void StringTables::OnCreate(const CSVCMsg_CreateStringTable& Msg)
    {
        Table T;
        T.Name             = Msg.name();
        T.UserDataFixed    = Msg.user_data_fixed_size();
        T.UserDataSizeBits = Msg.user_data_size_bits();
        T.Flags            = Msg.flags();
        T.VarintBitCounts  = Msg.using_varint_bitcounts();
        Tables.push_back(std::move(T));

        Table& Ref = Tables.back();
        if (Ref.Name != "instancebaseline")
            return;

        const std::string& Raw = Msg.string_data();
        Log("NetDelta instancebaseline create size %zu compressed %d uncompressed %d entries %d flags %d varint %d", Raw.size(),
            Msg.data_compressed() ? 1 : 0, static_cast<int>(Msg.uncompressed_size()), static_cast<int>(Msg.num_entries()), static_cast<int>(Msg.flags()),
            Msg.using_varint_bitcounts() ? 1 : 0);
        if (Msg.data_compressed())
        {
            std::vector<uint8_t> Plain;
            if (!SnappyDecompress(reinterpret_cast<const uint8_t*>(Raw.data()), Raw.size(), Plain,
                                  static_cast<size_t>(Msg.uncompressed_size())))
            {
                LogE("NetDelta instancebaseline snappy failed");
                return;
            }
            Parse(Ref, Plain.data(), Plain.size(), Msg.num_entries());
        }
        else
        {
            Parse(Ref, reinterpret_cast<const uint8_t*>(Raw.data()), Raw.size(), Msg.num_entries());
        }
        Log("NetDelta instancebaseline created entries %d baselines %zu", static_cast<int>(Msg.num_entries()), Baselines.size());
    }

    void StringTables::OnUpdate(const CSVCMsg_UpdateStringTable& Msg)
    {
        int Id = Msg.table_id();
        if (Id < 0 || Id >= static_cast<int>(Tables.size()))
            return;
        Table& T = Tables[Id];
        if (T.Name != "instancebaseline")
            return;
        const std::string& Raw = Msg.string_data();
        Parse(T, reinterpret_cast<const uint8_t*>(Raw.data()), Raw.size(), Msg.num_changed_entries());
    }

    void StringTables::Parse(Table& T, const uint8_t* Data, size_t Size, int Count)
    {
        if (!Data || Size == 0)
            return;

        BitReader R(Data, Size);
        int Index = -1;
        std::vector<std::string> History;

        for (int I = 0; I < Count && !R.Overflow(); ++I)
        {
            if (R.ReadBool())
                ++Index;
            else
                Index = static_cast<int>(R.ReadVarUint32()) + 1;

            std::string Key;
            if (R.ReadBool())
            {
                if (R.ReadBool())
                {
                    uint32_t Pos  = R.ReadBits(5);
                    uint32_t Len  = R.ReadBits(5);
                    if (Pos < History.size())
                    {
                        const std::string& S = History[Pos];
                        Key = Len > S.size() ? S : S.substr(0, Len);
                    }
                    Key += R.ReadString();
                }
                else
                {
                    Key = R.ReadString();
                }
                if (History.size() >= 32)
                    History.erase(History.begin());
                History.push_back(Key);
            }

            if (!Key.empty())
                T.Keys[Index] = Key;

            if (R.ReadBool())
            {
                uint32_t Bits = 0;
                bool Compressed = false;
                if (T.UserDataFixed)
                {
                    Bits = static_cast<uint32_t>(T.UserDataSizeBits);
                }
                else
                {
                    if (T.Flags & 1)
                        Compressed = R.ReadBool();
                    if (T.VarintBitCounts)
                        Bits = R.ReadUBitVar() * 8;
                    else
                        Bits = R.ReadBits(17) * 8;
                }

                if (static_cast<uint64_t>(Bits) > R.Remaining())
                {
                    LogE("NetDelta string table value overflow bits %llu", static_cast<unsigned long long>(Bits));
                    return;
                }
                std::vector<uint8_t> Value((Bits + 7) / 8, 0);
                for (uint32_t B = 0; B < Bits; B += 8)
                {
                    uint32_t Take = Bits - B >= 8 ? 8 : Bits - B;
                    Value[B / 8] = static_cast<uint8_t>(R.ReadBits(Take));
                }

                if (Compressed)
                {
                    std::vector<uint8_t> Plain;
                    size_t Offset = 0;
                    uint32_t Expected = 0;
                    if (!ReadVarUint32Bytes(Value.data(), Value.size(), Offset, Expected) ||
                        !SnappyDecompress(Value.data(), Value.size(), Plain, Expected))
                        continue;
                    Value.swap(Plain);
                }

                auto Name = T.Keys.find(Index);
                if (Name != T.Keys.end() && !Name->second.empty())
                    Baselines[std::atoi(Name->second.c_str())] = std::move(Value);
            }
        }
    }
}
