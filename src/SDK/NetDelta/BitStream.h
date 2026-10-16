#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace NetDelta
{
    class BitReader
    {
    public:
        BitReader() = default;
        BitReader(const uint8_t* Data, size_t SizeBytes);
        BitReader(const uint8_t* Data, size_t SizeBytes, size_t StartBit, size_t EndBit);

        uint32_t ReadBits(uint32_t Count);
        uint64_t ReadBits64(uint32_t Count);
        bool     ReadBool();
        uint32_t ReadUBitVar();
        uint32_t ReadUBitVarFP();
        uint32_t ReadVarUint32();
        uint64_t ReadVarUint64();
        int32_t  ReadVarInt32();
        int64_t  ReadVarInt64();
        std::string ReadString();
        float    ReadCoord();
        float    ReadAngle(uint32_t Bits);
        float    ReadNormal();
        float    ReadFloat();

        size_t Position() const { return Pos; }
        size_t End() const { return EndBit; }
        size_t Remaining() const { return Pos <= EndBit ? EndBit - Pos : 0; }
        void   Seek(size_t Bit) { Pos = Bit; }
        bool   Overflow() const { return Overflowed; }
        const uint8_t* Data() const { return Bytes; }

    private:
        const uint8_t* Bytes = nullptr;
        size_t Pos = 0;
        size_t EndBit = 0;
        bool   Overflowed = false;
    };

    class BitWriter
    {
    public:
        void WriteBits(uint64_t Value, uint32_t Count);
        void WriteBool(bool Value);
        void WriteUBitVar(uint32_t Value);
        void WriteUBitVarFP(uint32_t Value);
        void WriteVarUint32(uint32_t Value);
        void WriteVarUint64(uint64_t Value);
        void WriteVarInt32(int32_t Value);
        void WriteVarInt64(int64_t Value);
        void WriteString(const std::string& Value);
        void WriteFloat(float Value);
        void WriteCoord(float Value);
        void CopyBits(const uint8_t* Src, size_t StartBit, size_t Count);

        size_t Position() const { return Pos; }
        std::vector<uint8_t>& Buffer() { return Bytes; }
        const std::vector<uint8_t>& Buffer() const { return Bytes; }

    private:
        std::vector<uint8_t> Bytes;
        size_t Pos = 0;
    };

    void AppendVarUint32(std::vector<uint8_t>& Out, uint32_t Value);
    bool ReadVarUint32Bytes(const uint8_t* Data, size_t Size, size_t& Offset, uint32_t& Out);
}
