#include "BitStream.h"
#include <cmath>
#include <cstring>

namespace NetDelta
{
    BitReader::BitReader(const uint8_t* Data, size_t SizeBytes)
        : Bytes(Data), Pos(0), EndBit(SizeBytes * 8)
    {
    }

    BitReader::BitReader(const uint8_t* Data, size_t SizeBytes, size_t StartBit, size_t End)
        : Bytes(Data), Pos(StartBit), EndBit(End)
    {
        if (EndBit > SizeBytes * 8)
            EndBit = SizeBytes * 8;
    }

    uint32_t BitReader::ReadBits(uint32_t Count)
    {
        return static_cast<uint32_t>(ReadBits64(Count));
    }

    uint64_t BitReader::ReadBits64(uint32_t Count)
    {
        if (Count == 0)
            return 0;
        if (Pos + Count > EndBit)
        {
            Overflowed = true;
            Pos = EndBit;
            return 0;
        }

        uint64_t Result = 0;
        uint32_t Done = 0;
        while (Done < Count)
        {
            size_t   ByteIndex = Pos >> 3;
            uint32_t BitIndex  = static_cast<uint32_t>(Pos & 7);
            uint32_t Take      = 8 - BitIndex;
            if (Take > Count - Done)
                Take = Count - Done;
            uint64_t Chunk = (Bytes[ByteIndex] >> BitIndex) & ((1u << Take) - 1);
            Result |= Chunk << Done;
            Done += Take;
            Pos  += Take;
        }
        return Result;
    }

    bool BitReader::ReadBool()
    {
        return ReadBits(1) != 0;
    }

    uint32_t BitReader::ReadUBitVar()
    {
        uint32_t Ret = ReadBits(6);
        switch (Ret & 0x30)
        {
        case 16: Ret = (Ret & 15) | (ReadBits(4) << 4); break;
        case 32: Ret = (Ret & 15) | (ReadBits(8) << 4); break;
        case 48: Ret = (Ret & 15) | (ReadBits(28) << 4); break;
        default: break;
        }
        return Ret;
    }

    uint32_t BitReader::ReadUBitVarFP()
    {
        if (ReadBool()) return ReadBits(2);
        if (ReadBool()) return ReadBits(4);
        if (ReadBool()) return ReadBits(10);
        if (ReadBool()) return ReadBits(17);
        return ReadBits(31);
    }

    uint32_t BitReader::ReadVarUint32()
    {
        uint32_t Result = 0;
        for (uint32_t Shift = 0; Shift < 35; Shift += 7)
        {
            uint32_t Byte = ReadBits(8);
            Result |= (Byte & 0x7F) << Shift;
            if ((Byte & 0x80) == 0 || Overflowed)
                break;
        }
        return Result;
    }

    uint64_t BitReader::ReadVarUint64()
    {
        uint64_t Result = 0;
        for (uint32_t Shift = 0; Shift < 70; Shift += 7)
        {
            uint64_t Byte = ReadBits(8);
            Result |= (Byte & 0x7F) << Shift;
            if ((Byte & 0x80) == 0 || Overflowed)
                break;
        }
        return Result;
    }

    int32_t BitReader::ReadVarInt32()
    {
        uint32_t U = ReadVarUint32();
        return static_cast<int32_t>((U >> 1) ^ (~(U & 1) + 1));
    }

    int64_t BitReader::ReadVarInt64()
    {
        uint64_t U = ReadVarUint64();
        return static_cast<int64_t>((U >> 1) ^ (~(U & 1) + 1));
    }

    std::string BitReader::ReadString()
    {
        std::string Out;
        while (!Overflowed)
        {
            char C = static_cast<char>(ReadBits(8));
            if (C == 0)
                break;
            Out.push_back(C);
        }
        return Out;
    }

    float BitReader::ReadCoord()
    {
        float Value = 0.0f;
        uint32_t IntVal   = ReadBits(1);
        uint32_t FractVal = ReadBits(1);
        if (IntVal || FractVal)
        {
            bool Sign = ReadBool();
            if (IntVal)
                IntVal = ReadBits(14) + 1;
            if (FractVal)
                FractVal = ReadBits(5);
            Value = static_cast<float>(IntVal) + static_cast<float>(FractVal) * (1.0f / 32.0f);
            if (Sign)
                Value = -Value;
        }
        return Value;
    }

    float BitReader::ReadAngle(uint32_t Bits)
    {
        return static_cast<float>(ReadBits(Bits)) * 360.0f / static_cast<float>(1u << Bits);
    }

    float BitReader::ReadNormal()
    {
        bool     Neg = ReadBool();
        uint32_t Len = ReadBits(11);
        float    Ret = static_cast<float>(Len) * (1.0f / (static_cast<float>(1 << 11) - 1.0f));
        return Neg ? -Ret : Ret;
    }

    float BitReader::ReadFloat()
    {
        uint32_t Raw = ReadBits(32);
        float F;
        std::memcpy(&F, &Raw, 4);
        return F;
    }

    void BitWriter::WriteBits(uint64_t Value, uint32_t Count)
    {
        for (uint32_t Done = 0; Done < Count;)
        {
            size_t   ByteIndex = Pos >> 3;
            uint32_t BitIndex  = static_cast<uint32_t>(Pos & 7);
            if (ByteIndex >= Bytes.size())
                Bytes.push_back(0);
            uint32_t Take = 8 - BitIndex;
            if (Take > Count - Done)
                Take = Count - Done;
            uint8_t Chunk = static_cast<uint8_t>((Value >> Done) & ((1u << Take) - 1));
            Bytes[ByteIndex] |= static_cast<uint8_t>(Chunk << BitIndex);
            Done += Take;
            Pos  += Take;
        }
    }

    void BitWriter::WriteBool(bool Value)
    {
        WriteBits(Value ? 1 : 0, 1);
    }

    void BitWriter::WriteUBitVar(uint32_t Value)
    {
        if (Value < 16)
        {
            WriteBits(Value, 6);
        }
        else if (Value < 256)
        {
            WriteBits((Value & 15) | 16, 6);
            WriteBits(Value >> 4, 4);
        }
        else if (Value < 4096)
        {
            WriteBits((Value & 15) | 32, 6);
            WriteBits(Value >> 4, 8);
        }
        else
        {
            WriteBits((Value & 15) | 48, 6);
            WriteBits(Value >> 4, 28);
        }
    }

    void BitWriter::WriteUBitVarFP(uint32_t Value)
    {
        if (Value < 4)
        {
            WriteBool(true);
            WriteBits(Value, 2);
        }
        else if (Value < 16)
        {
            WriteBool(false);
            WriteBool(true);
            WriteBits(Value, 4);
        }
        else if (Value < 1024)
        {
            WriteBool(false);
            WriteBool(false);
            WriteBool(true);
            WriteBits(Value, 10);
        }
        else if (Value < (1u << 17))
        {
            WriteBool(false);
            WriteBool(false);
            WriteBool(false);
            WriteBool(true);
            WriteBits(Value, 17);
        }
        else
        {
            WriteBits(0, 4);
            WriteBits(Value, 31);
        }
    }

    void BitWriter::WriteVarUint32(uint32_t Value)
    {
        do
        {
            uint32_t Byte = Value & 0x7F;
            Value >>= 7;
            if (Value)
                Byte |= 0x80;
            WriteBits(Byte, 8);
        } while (Value);
    }

    void BitWriter::WriteVarUint64(uint64_t Value)
    {
        do
        {
            uint64_t Byte = Value & 0x7F;
            Value >>= 7;
            if (Value)
                Byte |= 0x80;
            WriteBits(Byte, 8);
        } while (Value);
    }

    void BitWriter::WriteVarInt32(int32_t Value)
    {
        WriteVarUint32((static_cast<uint32_t>(Value) << 1) ^ static_cast<uint32_t>(Value >> 31));
    }

    void BitWriter::WriteVarInt64(int64_t Value)
    {
        WriteVarUint64((static_cast<uint64_t>(Value) << 1) ^ static_cast<uint64_t>(Value >> 63));
    }

    void BitWriter::WriteString(const std::string& Value)
    {
        for (char C : Value)
            WriteBits(static_cast<uint8_t>(C), 8);
        WriteBits(0, 8);
    }

    void BitWriter::WriteFloat(float Value)
    {
        uint32_t Raw;
        std::memcpy(&Raw, &Value, 4);
        WriteBits(Raw, 32);
    }

    void BitWriter::WriteCoord(float Value)
    {
        bool  Sign = Value < 0.0f;
        float Abs  = std::fabs(Value);
        uint32_t IntVal   = static_cast<uint32_t>(Abs);
        uint32_t FractVal = static_cast<uint32_t>((Abs - static_cast<float>(IntVal)) * 32.0f) & 31;
        WriteBits(IntVal ? 1 : 0, 1);
        WriteBits(FractVal ? 1 : 0, 1);
        if (IntVal || FractVal)
        {
            WriteBool(Sign);
            if (IntVal)
                WriteBits(IntVal - 1, 14);
            if (FractVal)
                WriteBits(FractVal, 5);
        }
    }

    void BitWriter::CopyBits(const uint8_t* Src, size_t StartBit, size_t Count)
    {
        size_t Bit = StartBit;
        while (Count > 0)
        {
            uint32_t Take = Count > 32 ? 32 : static_cast<uint32_t>(Count);
            uint64_t Value = 0;
            for (uint32_t I = 0; I < Take; ++I)
            {
                size_t B = Bit + I;
                Value |= static_cast<uint64_t>((Src[B >> 3] >> (B & 7)) & 1) << I;
            }
            WriteBits(Value, Take);
            Bit   += Take;
            Count -= Take;
        }
    }

    void AppendVarUint32(std::vector<uint8_t>& Out, uint32_t Value)
    {
        do
        {
            uint8_t Byte = Value & 0x7F;
            Value >>= 7;
            if (Value)
                Byte |= 0x80;
            Out.push_back(Byte);
        } while (Value);
    }

    bool ReadVarUint32Bytes(const uint8_t* Data, size_t Size, size_t& Offset, uint32_t& Out)
    {
        Out = 0;
        for (uint32_t Shift = 0; Shift < 35; Shift += 7)
        {
            if (Offset >= Size)
                return false;
            uint8_t Byte = Data[Offset++];
            Out |= static_cast<uint32_t>(Byte & 0x7F) << Shift;
            if ((Byte & 0x80) == 0)
                return true;
        }
        return true;
    }
}
