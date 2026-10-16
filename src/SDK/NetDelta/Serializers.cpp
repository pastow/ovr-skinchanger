#include "Serializers.h"
#include "SDK/Logger/Logger.h"
#include "netmessages.pb.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <unordered_set>

namespace NetDelta
{
    namespace
    {
        constexpr uint32_t QffRoundDown      = 1u << 0;
        constexpr uint32_t QffRoundUp        = 1u << 1;
        constexpr uint32_t QffEncodeZero     = 1u << 2;
        constexpr uint32_t QffEncodeIntegers = 1u << 3;

        const std::unordered_set<std::string>& PointerTypes()
        {
            static const std::unordered_set<std::string> Set = {
                "PhysicsRagdollPose_t", "CBodyComponent", "CEntityIdentity", "CPhysicsComponent",
                "CRenderComponent", "CDOTAGamerules", "CDOTAGameManager", "CDOTASpectatorGraphManager",
                "CPlayerLocalData", "CPlayer_CameraServices", "CDOTAGameRules"
            };
            return Set;
        }

        std::string Trim(const std::string& S)
        {
            size_t B = S.find_first_not_of(' ');
            size_t E = S.find_last_not_of(' ');
            if (B == std::string::npos)
                return {};
            return S.substr(B, E - B + 1);
        }

        void ParseType(const std::string& Raw, FieldType& Out)
        {
            std::string Name = Trim(Raw);

            size_t Bracket = std::string::npos;
            int Depth = 0;
            for (size_t I = 0; I < Name.size(); ++I)
            {
                if (Name[I] == '<') ++Depth;
                else if (Name[I] == '>') --Depth;
                else if (Name[I] == '[' && Depth == 0) { Bracket = I; break; }
            }
            if (Bracket != std::string::npos)
            {
                size_t Close = Name.find(']', Bracket);
                std::string CountStr = Name.substr(Bracket + 1, Close - Bracket - 1);
                if (CountStr == "MAX_ITEM_STOCKS")
                    Out.Count = 8;
                else if (CountStr == "MAX_ABILITY_DRAFT_ABILITIES")
                    Out.Count = 48;
                else
                    Out.Count = std::atoi(CountStr.c_str());
                Name = Trim(Name.substr(0, Bracket));
            }

            if (!Name.empty() && Name.back() == '*')
            {
                Out.Pointer = true;
                Name = Trim(Name.substr(0, Name.size() - 1));
            }

            size_t Lt = Name.find('<');
            if (Lt != std::string::npos)
            {
                size_t Gt = Name.rfind('>');
                std::string Inner = Name.substr(Lt + 1, Gt - Lt - 1);
                Out.Generic = std::make_unique<FieldType>();
                ParseType(Inner, *Out.Generic);
                Name = Trim(Name.substr(0, Lt));
            }

            Out.Base = Name;
        }

        Decoder FloatDecoder(const Field& F)
        {
            Decoder D;
            if (F.Encoder == "coord") { D.Kind = DecoderKind::Coord; return D; }
            if (F.Encoder == "simtime") { D.Kind = DecoderKind::SimTime; return D; }
            if (F.Encoder == "runetime") { D.Kind = DecoderKind::RuneTime; return D; }
            if (!F.HasBitCount)
            {
                D.Kind = DecoderKind::SimTime;
                return D;
            }
            if (F.BitCount <= 0 || F.BitCount >= 32)
            {
                D.Kind = DecoderKind::NoScale;
                return D;
            }
            D.Kind = DecoderKind::Quantized;
            D.Q.Init(F.BitCount, F.HasFlags ? F.Flags : 0, F.HasLow ? F.Low : 0.0f, F.HasHigh ? F.High : 1.0f);
            return D;
        }

        Decoder MakeDecoder(const Field& F, const std::string& BaseType)
        {
            Decoder D;
            if (F.Encoder == "fixed8")
            {
                D.Kind = DecoderKind::Fixed8;
                return D;
            }
            if (BaseType == "float32" || BaseType == "CNetworkedQuantizedFloat")
                return FloatDecoder(F);

            if (BaseType == "Vector" || BaseType == "VectorWS" || BaseType == "Vector2D" || BaseType == "Vector4D" ||
                BaseType == "Quaternion")
            {
                uint32_t N = BaseType == "Vector2D" ? 2 : ((BaseType == "Vector" || BaseType == "VectorWS") ? 3 : 4);
                if (N == 3 && F.Encoder == "normal")
                {
                    D.Kind = DecoderKind::Normal;
                    D.Count = 3;
                    return D;
                }
                Decoder Inner = FloatDecoder(F);
                D.Kind  = DecoderKind::Vector;
                D.Inner = Inner.Kind;
                D.Q     = Inner.Q;
                D.Count = N;
                return D;
            }

            if (BaseType == "QAngle")
            {
                if (F.Encoder == "qangle_pitch_yaw")
                {
                    D.Kind = DecoderKind::QAnglePitchYaw;
                    D.Bits = static_cast<uint32_t>(F.BitCount);
                    return D;
                }
                if (F.HasBitCount && F.BitCount != 0)
                {
                    D.Kind = DecoderKind::Angle;
                    D.Bits = static_cast<uint32_t>(F.BitCount);
                    D.Count = 3;
                    return D;
                }
                D.Kind = DecoderKind::QAnglePrecise;
                return D;
            }

            if (BaseType == "uint64" || BaseType == "CStrongHandle")
            {
                D.Kind = F.Encoder == "fixed64" ? DecoderKind::Fixed64 : DecoderKind::VarUint64;
                return D;
            }

            if (BaseType == "bool")
            {
                D.Kind = DecoderKind::Bool;
                return D;
            }

            if (BaseType == "char" || BaseType == "CUtlString" || BaseType == "CUtlSymbolLarge")
            {
                D.Kind = DecoderKind::String;
                return D;
            }

            if (BaseType == "int8" || BaseType == "int16" || BaseType == "int32")
            {
                D.Kind = DecoderKind::VarInt32;
                return D;
            }

            if (BaseType == "int64")
            {
                D.Kind = DecoderKind::VarInt64;
                return D;
            }

            if (BaseType == "GameTime_t")
            {
                D.Kind = DecoderKind::NoScale;
                return D;
            }

            if (PointerTypes().count(BaseType))
            {
                D.Kind = DecoderKind::Bool;
                return D;
            }

            D.Kind = DecoderKind::VarUint64;
            return D;
        }

        void AssignModel(Field& F)
        {
            if (F.Child)
            {
                if (F.Type.Pointer || PointerTypes().count(F.Type.Base))
                {
                    F.Model = FieldModel::FixedTable;
                    F.Base.Kind = DecoderKind::Bool;
                }
                else
                {
                    F.Model = FieldModel::VariableTable;
                    F.Base.Kind = DecoderKind::VarUint32;
                }
                return;
            }

            if (F.Type.Count > 0 && F.Type.Base != "char")
            {
                F.Model = FieldModel::FixedArray;
                F.Value = MakeDecoder(F, F.Type.Base);
                return;
            }

            if (F.Type.Base == "CUtlVector" || F.Type.Base == "CNetworkUtlVectorBase")
            {
                F.Model = FieldModel::VariableArray;
                F.Base.Kind = DecoderKind::VarUint32;
                F.Element = MakeDecoder(F, F.Type.Generic ? F.Type.Generic->Base : std::string{});
                return;
            }

            F.Model = FieldModel::Simple;
            F.Value = MakeDecoder(F, F.Type.Base);
        }

        bool ReadFloatKind(BitReader& R, DecoderKind Kind, const QuantizedFloat& Q, float& Out)
        {
            switch (Kind)
            {
            case DecoderKind::NoScale:   Out = R.ReadFloat(); return true;
            case DecoderKind::Quantized: Out = Q.Decode(R); return true;
            case DecoderKind::Coord:     Out = R.ReadCoord(); return true;
            case DecoderKind::SimTime:   Out = static_cast<float>(R.ReadVarUint32()) * (1.0f / 30.0f); return true;
            case DecoderKind::RuneTime:
            {
                uint32_t Raw = R.ReadBits(4);
                float V;
                std::memcpy(&V, &Raw, 4);
                Out = V;
                return true;
            }
            default:
                return false;
            }
        }

        bool WriteFloatKind(BitWriter& W, DecoderKind Kind, const QuantizedFloat& Q, float Value)
        {
            switch (Kind)
            {
            case DecoderKind::NoScale:   W.WriteFloat(Value); return true;
            case DecoderKind::Quantized: Q.Encode(W, Value); return true;
            case DecoderKind::Coord:     W.WriteCoord(Value); return true;
            case DecoderKind::SimTime:
            {
                float Ticks = Value * 30.0f;
                W.WriteVarUint32(Ticks <= 0.0f ? 0u : static_cast<uint32_t>(Ticks + 0.5f));
                return true;
            }
            default:
                return false;
            }
        }
    }

    void QuantizedFloat::Init(int32_t BitCountIn, int32_t InFlags, float InLow, float InHigh)
    {
        if (BitCountIn == 0 || BitCountIn >= 32)
        {
            NoScale  = true;
            Bitcount = 32;
            return;
        }

        NoScale  = false;
        Bitcount = static_cast<uint32_t>(BitCountIn);
        Offset   = 0.0f;
        Low      = InLow;
        High     = InHigh;
        Flags    = static_cast<uint32_t>(InFlags);

        if (Flags != 0)
        {
            if ((Low == 0.0f && (Flags & QffRoundDown)) || (High == 0.0f && (Flags & QffRoundUp)))
                Flags &= ~QffEncodeZero;
            if (Low == 0.0f && (Flags & QffEncodeZero))
            {
                Flags |= QffRoundDown;
                Flags &= ~QffEncodeZero;
            }
            if (High == 0.0f && (Flags & QffEncodeZero))
            {
                Flags |= QffRoundUp;
                Flags &= ~QffEncodeZero;
            }
            if (Low > 0.0f || High < 0.0f)
                Flags &= ~QffEncodeZero;
            if (Flags & QffEncodeIntegers)
                Flags &= ~(QffRoundUp | QffRoundDown | QffEncodeZero);
        }

        uint32_t Steps = 1u << Bitcount;
        if (Flags & QffRoundDown)
        {
            float Range = High - Low;
            Offset = Range / static_cast<float>(Steps);
            High -= Offset;
        }
        else if (Flags & QffRoundUp)
        {
            float Range = High - Low;
            Offset = Range / static_cast<float>(Steps);
            Low += Offset;
        }

        if (Flags & QffEncodeIntegers)
        {
            float Delta = High - Low;
            if (Delta < 1.0f)
                Delta = 1.0f;
            double DeltaLog2 = std::ceil(std::log2(static_cast<double>(Delta)));
            uint32_t Range2 = 1u << static_cast<uint32_t>(DeltaLog2);
            uint32_t Bc = Bitcount;
            while ((1u << Bc) <= Range2)
                ++Bc;
            if (Bc > Bitcount)
            {
                Bitcount = Bc;
                Steps = 1u << Bitcount;
            }
            Offset = static_cast<float>(Range2) / static_cast<float>(Steps);
            High = Low + static_cast<float>(Range2) - Offset;
        }

        float Range = High - Low;
        uint32_t HighU = Bitcount == 32 ? 0xFFFFFFFEu : (1u << Bitcount) - 1;
        float HighMul;
        if (std::fabs(Range) <= 0.0f)
            HighMul = static_cast<float>(HighU);
        else
            HighMul = static_cast<float>(HighU) / Range;

        if (HighMul * Range > static_cast<float>(HighU) ||
            static_cast<double>(HighMul * Range) > static_cast<double>(HighU))
        {
            const float Multipliers[] = { 0.9999f, 0.99f, 0.9f, 0.8f, 0.7f };
            for (float Mult : Multipliers)
            {
                HighMul = static_cast<float>(HighU) / Range * Mult;
                if (HighMul * Range > static_cast<float>(HighU) ||
                    static_cast<double>(HighMul * Range) > static_cast<double>(HighU))
                    continue;
                break;
            }
        }

        HighLowMul = HighMul;
        DecMul = 1.0f / static_cast<float>(Steps - 1);

        if ((Flags & QffRoundDown) && Quantize(Low) == Low)
            Flags &= ~QffRoundDown;
        if ((Flags & QffRoundUp) && Quantize(High) == High)
            Flags &= ~QffRoundUp;
        if ((Flags & QffEncodeZero) && Quantize(0.0f) == 0.0f)
            Flags &= ~QffEncodeZero;
    }

    float QuantizedFloat::Quantize(float Value) const
    {
        if (Value < Low)
            return Low;
        if (Value > High)
            return High;
        uint32_t I = static_cast<uint32_t>((Value - Low) * HighLowMul);
        return Low + (High - Low) * (static_cast<float>(I) * DecMul);
    }

    float QuantizedFloat::Decode(BitReader& R) const
    {
        if (NoScale)
            return R.ReadFloat();
        if ((Flags & QffRoundDown) && R.ReadBool())
            return Low;
        if ((Flags & QffRoundUp) && R.ReadBool())
            return High;
        if ((Flags & QffEncodeZero) && R.ReadBool())
            return 0.0f;
        return Low + (High - Low) * static_cast<float>(R.ReadBits(Bitcount)) * DecMul;
    }

    void QuantizedFloat::Encode(BitWriter& W, float Value) const
    {
        if (NoScale)
        {
            W.WriteFloat(Value);
            return;
        }
        if (Flags & QffRoundDown)
        {
            bool IsLow = Value <= Low;
            W.WriteBool(IsLow);
            if (IsLow)
                return;
        }
        if (Flags & QffRoundUp)
        {
            bool IsHigh = Value >= High;
            W.WriteBool(IsHigh);
            if (IsHigh)
                return;
        }
        if (Flags & QffEncodeZero)
        {
            bool IsZero = Value == 0.0f;
            W.WriteBool(IsZero);
            if (IsZero)
                return;
        }
        float Clamped = Value < Low ? Low : (Value > High ? High : Value);
        uint32_t Max = (1u << Bitcount) - 1;
        float Scaled = (Clamped - Low) / (High - Low) * static_cast<float>(Max) + 0.5f;
        uint32_t Bits = Scaled <= 0.0f ? 0u : static_cast<uint32_t>(Scaled);
        if (Bits > Max)
            Bits = Max;
        W.WriteBits(Bits, Bitcount);
    }

    bool ReadValue(BitReader& R, const Decoder& D, FieldValue* Out)
    {
        FieldValue Tmp;
        FieldValue& V = Out ? *Out : Tmp;
        V.Kind = D.Kind;

        switch (D.Kind)
        {
        case DecoderKind::Bool:      V.U = R.ReadBool() ? 1 : 0; break;
        case DecoderKind::VarUint32: V.U = R.ReadVarUint32(); break;
        case DecoderKind::VarInt32:  V.I = R.ReadVarInt32(); V.U = static_cast<uint64_t>(V.I); break;
        case DecoderKind::VarUint64: V.U = R.ReadVarUint64(); break;
        case DecoderKind::VarInt64:  V.I = R.ReadVarInt64(); V.U = static_cast<uint64_t>(V.I); break;
        case DecoderKind::Fixed64:   V.U = R.ReadBits64(64); break;
        case DecoderKind::Fixed8:    V.U = R.ReadBits(8); V.I = static_cast<int64_t>(V.U); break;
        case DecoderKind::String:
            if (Out)
                V.S = R.ReadString();
            else
                R.ReadString();
            break;
        case DecoderKind::NoScale:
        case DecoderKind::Quantized:
        case DecoderKind::Coord:
        case DecoderKind::SimTime:
        case DecoderKind::RuneTime:
            if (!ReadFloatKind(R, D.Kind, D.Q, V.F[0]))
                return false;
            break;
        case DecoderKind::Angle:
            for (uint32_t I = 0; I < D.Count && I < 4; ++I)
                V.F[I] = R.ReadAngle(D.Bits);
            break;
        case DecoderKind::QAnglePitchYaw:
            V.F[0] = R.ReadAngle(D.Bits);
            V.F[1] = R.ReadAngle(D.Bits);
            V.F[2] = 0.0f;
            break;
        case DecoderKind::QAnglePrecise:
        {
            bool Rx = R.ReadBool();
            bool Ry = R.ReadBool();
            bool Rz = R.ReadBool();
            V.F[0] = Rx ? R.ReadCoord() : 0.0f;
            V.F[1] = Ry ? R.ReadCoord() : 0.0f;
            V.F[2] = Rz ? R.ReadCoord() : 0.0f;
            break;
        }
        case DecoderKind::Normal:
        {
            bool HasX = R.ReadBool();
            bool HasY = R.ReadBool();
            V.F[0] = HasX ? R.ReadNormal() : 0.0f;
            V.F[1] = HasY ? R.ReadNormal() : 0.0f;
            bool NegZ = R.ReadBool();
            float Prod = V.F[0] * V.F[0] + V.F[1] * V.F[1];
            V.F[2] = Prod < 1.0f ? std::sqrt(1.0f - Prod) : 0.0f;
            if (NegZ)
                V.F[2] = -V.F[2];
            break;
        }
        case DecoderKind::Vector:
            for (uint32_t I = 0; I < D.Count && I < 4; ++I)
                if (!ReadFloatKind(R, D.Inner, D.Q, V.F[I]))
                    return false;
            break;
        default:
            return false;
        }
        return !R.Overflow();
    }

    bool WriteValue(BitWriter& W, const Decoder& D, const FieldValue& V)
    {
        switch (D.Kind)
        {
        case DecoderKind::Bool:      W.WriteBool(V.U != 0); return true;
        case DecoderKind::VarUint32: W.WriteVarUint32(static_cast<uint32_t>(V.U)); return true;
        case DecoderKind::VarInt32:  W.WriteVarInt32(static_cast<int32_t>(V.I)); return true;
        case DecoderKind::VarUint64: W.WriteVarUint64(V.U); return true;
        case DecoderKind::VarInt64:  W.WriteVarInt64(V.I); return true;
        case DecoderKind::Fixed64:   W.WriteBits(V.U, 64); return true;
        case DecoderKind::Fixed8:    W.WriteBits(V.U & 0xFF, 8); return true;
        case DecoderKind::String:    W.WriteString(V.S); return true;
        case DecoderKind::NoScale:
        case DecoderKind::Quantized:
        case DecoderKind::Coord:
        case DecoderKind::SimTime:
            return WriteFloatKind(W, D.Kind, D.Q, V.F[0]);
        case DecoderKind::Angle:
            for (uint32_t I = 0; I < D.Count && I < 4; ++I)
            {
                float Steps = static_cast<float>(1u << D.Bits);
                float A = std::fmod(V.F[I], 360.0f);
                if (A < 0.0f)
                    A += 360.0f;
                W.WriteBits(static_cast<uint32_t>(A * Steps / 360.0f + 0.5f) & ((1u << D.Bits) - 1), D.Bits);
            }
            return true;
        case DecoderKind::QAnglePitchYaw:
            for (int I = 0; I < 2; ++I)
            {
                float Steps = static_cast<float>(1u << D.Bits);
                float A = std::fmod(V.F[I], 360.0f);
                if (A < 0.0f)
                    A += 360.0f;
                W.WriteBits(static_cast<uint32_t>(A * Steps / 360.0f + 0.5f) & ((1u << D.Bits) - 1), D.Bits);
            }
            return true;
        case DecoderKind::QAnglePrecise:
            for (int I = 0; I < 3; ++I)
                W.WriteBool(V.F[I] != 0.0f);
            for (int I = 0; I < 3; ++I)
                if (V.F[I] != 0.0f)
                    W.WriteCoord(V.F[I]);
            return true;
        case DecoderKind::Vector:
            for (uint32_t I = 0; I < D.Count && I < 4; ++I)
                if (!WriteFloatKind(W, D.Inner, D.Q, V.F[I]))
                    return false;
            return true;
        default:
            return false;
        }
    }

    const Decoder* Serializer::GetDecoder(const FieldPath& Fp, int Pos) const
    {
        if (Pos > Fp.Last)
            return nullptr;
        int Index = Fp.Path[Pos];
        if (Index < 0 || Index >= static_cast<int>(Fields.size()))
            return nullptr;

        const Field* F = Fields[Index];
        int Next = Pos + 1;
        switch (F->Model)
        {
        case FieldModel::FixedArray:
            return &F->Value;
        case FieldModel::FixedTable:
            if (Fp.Last == Next - 1)
                return &F->Base;
            return F->Child ? F->Child->GetDecoder(Fp, Next) : nullptr;
        case FieldModel::VariableArray:
            if (Fp.Last == Next)
                return &F->Element;
            return &F->Base;
        case FieldModel::VariableTable:
            if (Fp.Last >= Next + 1)
                return F->Child ? F->Child->GetDecoder(Fp, Next + 1) : nullptr;
            return &F->Base;
        case FieldModel::Simple:
        default:
            return &F->Value;
        }
    }

    bool Serializer::GetName(const FieldPath& Fp, int Pos, std::string& Out) const
    {
        if (Pos > Fp.Last)
            return false;
        int Index = Fp.Path[Pos];
        if (Index < 0 || Index >= static_cast<int>(Fields.size()))
            return false;

        const Field* F = Fields[Index];
        Out += F->Name;
        int Next = Pos + 1;
        switch (F->Model)
        {
        case FieldModel::FixedArray:
        case FieldModel::VariableArray:
            if (Fp.Last == Next)
                Out += "." + std::to_string(Fp.Path[Next]);
            return true;
        case FieldModel::FixedTable:
            if (Fp.Last >= Next && F->Child)
            {
                Out += ".";
                return F->Child->GetName(Fp, Next, Out);
            }
            return true;
        case FieldModel::VariableTable:
            if (Fp.Last != Next - 1)
                Out += "." + std::to_string(Fp.Path[Next]);
            if (Fp.Last != Next && F->Child)
            {
                Out += ".";
                return F->Child->GetName(Fp, Next + 1, Out);
            }
            return true;
        default:
            return true;
        }
    }

    int Serializer::FieldIndex(const std::string& Name) const
    {
        for (size_t I = 0; I < Fields.size(); ++I)
            if (Fields[I]->Name == Name)
                return static_cast<int>(I);
        return -1;
    }

    bool Serializer::FindPath(const std::string& Dotted, FieldPath& Out) const
    {
        std::vector<std::string> Parts;
        size_t Start = 0;
        while (true)
        {
            size_t Dot = Dotted.find('.', Start);
            Parts.push_back(Dotted.substr(Start, Dot == std::string::npos ? std::string::npos : Dot - Start));
            if (Dot == std::string::npos)
                break;
            Start = Dot + 1;
        }

        FieldPath Fp;
        Fp.Last = -1;
        const Serializer* Cur = this;
        for (size_t I = 0; I < Parts.size(); ++I)
        {
            if (!Cur)
                return false;
            int Index = Cur->FieldIndex(Parts[I]);
            if (Index < 0 || Fp.Last >= 6)
                return false;
            Fp.Path[++Fp.Last] = Index;
            const Field* F = Cur->Fields[Index];

            if (F->Model == FieldModel::FixedTable)
            {
                Cur = F->Child;
                continue;
            }

            if ((F->Model == FieldModel::FixedArray || F->Model == FieldModel::VariableArray) && I + 1 < Parts.size())
            {
                if (Fp.Last >= 6)
                    return false;
                Fp.Path[++Fp.Last] = std::atoi(Parts[I + 1].c_str());
                ++I;
            }
            Cur = nullptr;
        }

        if (Fp.Last < 0)
            return false;
        Out = Fp;
        return true;
    }

    void SerializerRegistry::Clear()
    {
        FieldStorage.clear();
        SerializerStorage.clear();
        ByName.clear();
        Loaded = false;
    }

    const Serializer* SerializerRegistry::Find(const std::string& Name) const
    {
        auto It = ByName.find(Name);
        return It != ByName.end() ? It->second : nullptr;
    }

    bool SerializerRegistry::Load(const CSVCMsg_FlattenedSerializer& Msg)
    {
        Clear();

        auto Sym = [&Msg](int32_t Index) -> std::string
        {
            if (Index < 0 || Index >= Msg.symbols_size())
                return {};
            return Msg.symbols(Index);
        };

        std::unordered_map<std::string, Serializer*> ByNameVersion;
        SerializerStorage.reserve(Msg.serializers_size());
        for (const auto& S : Msg.serializers())
        {
            auto Ser = std::make_unique<Serializer>();
            Ser->Name    = Sym(S.serializer_name_sym());
            Ser->Version = S.serializer_version();
            ByNameVersion[Ser->Name + "#" + std::to_string(Ser->Version)] = Ser.get();
            ByName[Ser->Name] = Ser.get();
            SerializerStorage.push_back(std::move(Ser));
        }

        std::vector<Field*> FieldByIndex(Msg.fields_size(), nullptr);
        std::unordered_set<std::string> UnknownEncoders;
        int Polymorphic = 0;

        auto BuildField = [&](int Index) -> Field*
        {
            if (Index < 0 || Index >= Msg.fields_size())
                return nullptr;
            if (FieldByIndex[Index])
                return FieldByIndex[Index];

            const auto& P = Msg.fields(Index);
            auto F = std::make_unique<Field>();
            F->Name    = Sym(P.var_name_sym());
            F->VarType = Sym(P.var_type_sym());
            if (P.has_var_encoder_sym())
                F->Encoder = Sym(P.var_encoder_sym());
            F->HasBitCount = P.has_bit_count();
            F->BitCount    = P.bit_count();
            F->HasLow      = P.has_low_value();
            F->Low         = P.low_value();
            F->HasHigh     = P.has_high_value();
            F->High        = P.high_value();
            F->HasFlags    = P.has_encode_flags();
            F->Flags       = P.encode_flags();
            ParseType(F->VarType, F->Type);

            if (P.has_field_serializer_name_sym())
            {
                F->SerializerName    = Sym(P.field_serializer_name_sym());
                F->SerializerVersion = P.field_serializer_version();
                auto It = ByNameVersion.find(F->SerializerName + "#" + std::to_string(F->SerializerVersion));
                if (It != ByNameVersion.end())
                    F->Child = It->second;
                else
                {
                    auto Fallback = ByName.find(F->SerializerName);
                    if (Fallback != ByName.end())
                        F->Child = Fallback->second;
                }
            }

            for (const auto& Poly : P.polymorphic_types())
            {
                std::string Name = Sym(Poly.polymorphic_field_serializer_name_sym());
                auto It = ByNameVersion.find(Name + "#" + std::to_string(Poly.polymorphic_field_serializer_version()));
                if (It != ByNameVersion.end())
                    F->Polymorphic.push_back(It->second);
            }
            if (!F->Polymorphic.empty())
                ++Polymorphic;

            if (!F->Encoder.empty() && F->Encoder != "coord" && F->Encoder != "simtime" && F->Encoder != "runetime" &&
                F->Encoder != "normal" && F->Encoder != "fixed64" && F->Encoder != "qangle_pitch_yaw" &&
                F->Encoder != "fixed8" && F->Encoder != "qangle")
                UnknownEncoders.insert(F->Encoder + " (" + F->VarType + " " + F->Name + ")");

            AssignModel(*F);
            FieldByIndex[Index] = F.get();
            FieldStorage.push_back(std::move(F));
            return FieldByIndex[Index];
        };

        for (int I = 0; I < Msg.serializers_size(); ++I)
        {
            Serializer* Ser = SerializerStorage[I].get();
            for (int32_t FieldIndex : Msg.serializers(I).fields_index())
            {
                Field* F = BuildField(FieldIndex);
                if (!F)
                {
                    LogE("NetDelta bad field index %lld in %s", static_cast<long long>(FieldIndex), Ser->Name.c_str());
                    return false;
                }
                Ser->Fields.push_back(F);
            }
        }

        for (auto& F : FieldStorage)
        {
            if (!F->SerializerName.empty() && !F->Child)
            {
                auto It = ByName.find(F->SerializerName);
                if (It != ByName.end())
                {
                    F->Child = It->second;
                    AssignModel(*F);
                }
            }
        }

        for (const auto& E : UnknownEncoders)
            Log("NetDelta unknown encoder %s", std::string(E).c_str());

        Log("NetDelta serializers %zu fields %zu symbols %d polymorphic %llu", SerializerStorage.size(), FieldStorage.size(), Msg.symbols_size(),
            static_cast<unsigned long long>(Polymorphic));
        Loaded = true;
        return true;
    }
}
