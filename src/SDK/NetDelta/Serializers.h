#pragma once
#include "BitStream.h"
#include "FieldPath.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class CSVCMsg_FlattenedSerializer;

namespace NetDelta
{
    enum class DecoderKind : uint8_t
    {
        None,
        Bool,
        VarUint32,
        VarInt32,
        VarUint64,
        VarInt64,
        Fixed64,
        Fixed8,
        String,
        NoScale,
        Quantized,
        Coord,
        SimTime,
        RuneTime,
        Angle,
        QAnglePitchYaw,
        QAnglePrecise,
        Normal,
        Vector
    };

    struct QuantizedFloat
    {
        uint32_t Bitcount = 0;
        uint32_t Flags = 0;
        float    Low = 0.0f;
        float    High = 1.0f;
        float    HighLowMul = 0.0f;
        float    DecMul = 0.0f;
        float    Offset = 0.0f;
        bool     NoScale = false;

        void  Init(int32_t BitCount, int32_t InFlags, float InLow, float InHigh);
        float Quantize(float Value) const;
        float Decode(BitReader& R) const;
        void  Encode(BitWriter& W, float Value) const;
    };

    struct Decoder
    {
        DecoderKind Kind = DecoderKind::None;
        DecoderKind Inner = DecoderKind::None;
        uint32_t Bits = 0;
        uint32_t Count = 0;
        QuantizedFloat Q;
    };

    struct FieldValue
    {
        DecoderKind Kind = DecoderKind::None;
        uint64_t    U = 0;
        int64_t     I = 0;
        float       F[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        std::string S;
    };

    bool ReadValue(BitReader& R, const Decoder& D, FieldValue* Out);
    bool WriteValue(BitWriter& W, const Decoder& D, const FieldValue& Value);

    enum class FieldModel : uint8_t
    {
        Simple,
        FixedArray,
        FixedTable,
        VariableArray,
        VariableTable
    };

    struct FieldType
    {
        std::string Base;
        std::unique_ptr<FieldType> Generic;
        bool Pointer = false;
        int  Count = 0;
    };

    struct Serializer;

    struct Field
    {
        std::string Name;
        std::string VarType;
        std::string Encoder;
        FieldType   Type;
        int32_t     BitCount = 0;
        float       Low = 0.0f;
        float       High = 1.0f;
        int32_t     Flags = 0;
        bool        HasBitCount = false;
        bool        HasLow = false;
        bool        HasHigh = false;
        bool        HasFlags = false;
        std::string SerializerName;
        int32_t     SerializerVersion = 0;
        Serializer* Child = nullptr;
        std::vector<Serializer*> Polymorphic;
        FieldModel  Model = FieldModel::Simple;
        Decoder     Value;
        Decoder     Base;
        Decoder     Element;
    };

    struct Serializer
    {
        std::string Name;
        int32_t     Version = 0;
        std::vector<Field*> Fields;

        const Decoder* GetDecoder(const FieldPath& Fp, int Pos) const;
        bool GetName(const FieldPath& Fp, int Pos, std::string& Out) const;
        bool FindPath(const std::string& Dotted, FieldPath& Out) const;
        int  FieldIndex(const std::string& Name) const;
    };

    class SerializerRegistry
    {
    public:
        bool Load(const CSVCMsg_FlattenedSerializer& Msg);
        void Clear();
        bool Ready() const { return Loaded; }
        const Serializer* Find(const std::string& Name) const;
        size_t Count() const { return ByName.size(); }

    private:
        std::vector<std::unique_ptr<Field>> FieldStorage;
        std::vector<std::unique_ptr<Serializer>> SerializerStorage;
        std::unordered_map<std::string, Serializer*> ByName;
        bool Loaded = false;
    };
}
