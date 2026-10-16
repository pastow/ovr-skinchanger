#include "EntityWorld.h"
#include "SDK/Logger/Logger.h"
#include "netmessages.pb.h"
#include <algorithm>
#include <unordered_set>

namespace NetDelta
{
    namespace
    {
        struct Source
        {
            bool   FromOriginal = true;
            size_t Start = 0;
            size_t End = 0;
            FieldValue Value;
        };

        int Mismatched = 0;
        int Verified = 0;
        int VerifyFailures = 0;
    }

    const FieldValue* EntityState::Get(const std::string& Dotted) const
    {
        if (!Ser)
            return nullptr;
        FieldPath Fp;
        if (!Ser->FindPath(Dotted, Fp))
            return nullptr;
        auto It = Values.find(Fp.Key());
        return It != Values.end() ? &It->second : nullptr;
    }

    uint64_t EntityState::GetUint(const std::string& Dotted, uint64_t Default) const
    {
        const FieldValue* V = Get(Dotted);
        return V ? V->U : Default;
    }

    float EntityState::GetFloat(const std::string& Dotted, float Default) const
    {
        const FieldValue* V = Get(Dotted);
        return V ? V->F[0] : Default;
    }

    EntityWorld& EntityWorld::Instance()
    {
        static EntityWorld World;
        return World;
    }

    bool EntityWorld::Ready() const
    {
        return Registry.Ready() && ClassBits > 0 && !ClassNames.empty();
    }

    const EntityState* EntityWorld::Get(int Index) const
    {
        if (Index < 0 || Index >= MaxEntities)
            return nullptr;
        return Entities[Index].Active ? &Entities[Index] : nullptr;
    }

    EntityState* EntityWorld::GetMutable(int Index)
    {
        if (Index < 0 || Index >= MaxEntities)
            return nullptr;
        return Entities[Index].Active ? &Entities[Index] : nullptr;
    }

    std::string EntityWorld::ClassName(int ClassId) const
    {
        if (ClassId < 0 || ClassId >= static_cast<int>(ClassNames.size()))
            return {};
        return ClassNames[ClassId];
    }

    int EntityWorld::ClassIdByName(const std::string& Name) const
    {
        auto It = ClassIds.find(Name);
        return It != ClassIds.end() ? It->second : -1;
    }

    const Serializer* EntityWorld::ClassSerializer(int ClassId) const
    {
        std::string Name = ClassName(ClassId);
        return Name.empty() ? nullptr : Registry.Find(Name);
    }

    bool EntityWorld::IsTracked(const Serializer* Ser) const
    {
        return Ser && TrackPred && TrackPred(Ser->Name);
    }

    void EntityWorld::OnServerInfo(const CSVCMsg_ServerInfo& Msg)
    {
        MaxClasses = Msg.max_classes();
        ClassBits = 1;
        if (MaxClasses > 1)
        {
            uint32_t V = static_cast<uint32_t>(MaxClasses - 1);
            ClassBits = 0;
            while (V)
            {
                ++ClassBits;
                V >>= 1;
            }
        }
        if (Msg.tick_interval() > 0.0f)
            Interval = Msg.tick_interval();
        for (auto& E : Entities)
            E = EntityState{};
        ClassNames.clear();
        ClassIds.clear();
        ++Connections;
        Log("NetDelta serverinfo max_classes %d class_bits %u tick_interval %f", MaxClasses, ClassBits, static_cast<double>(Interval));
        RebuildClassMap();
    }

    void EntityWorld::OnClassInfo(const CSVCMsg_ClassInfo& Msg)
    {
        ClassInfoReceived = true;
        CreateOnClient = Msg.create_on_client();
        ClassNames.clear();
        ClassIds.clear();
        for (const auto& C : Msg.classes())
        {
            if (C.class_id() < 0 || (MaxClasses > 0 && C.class_id() >= MaxClasses))
            {
                LogE("NetDelta classinfo bad class id %d max %d", static_cast<int>(C.class_id()), MaxClasses);
                continue;
            }
            if (C.class_id() >= static_cast<int>(ClassNames.size()))
                ClassNames.resize(C.class_id() + 1);
            ClassNames[C.class_id()] = C.class_name();
            ClassIds[C.class_name()] = C.class_id();
        }
        Log("NetDelta classinfo create_on_client %d classes %d", CreateOnClient ? 1 : 0, Msg.classes_size());
        RebuildClassMap();
    }

    void EntityWorld::RebuildClassMap()
    {
        if (!ClassNames.empty() || !Registry.Ready())
            return;
        if (!ClassInfoReceived)
            return;
        LogE("NetDelta classinfo has no class list create_on_client %d", CreateOnClient ? 1 : 0);
    }

    void EntityWorld::OnSerializers(const CSVCMsg_FlattenedSerializer& Msg)
    {
        Registry.Load(Msg);
        RebuildClassMap();
    }

    bool EntityWorld::LoadClasses(const std::vector<std::string>& Names, const CSVCMsg_FlattenedSerializer& Msg)
    {
        Registry.Load(Msg);
        ClassNames = Names;
        ClassIds.clear();
        size_t Linked = 0;
        for (size_t I = 0; I < ClassNames.size(); ++I)
        {
            ClassIds[ClassNames[I]] = static_cast<int>(I);
            if (Registry.Find(ClassNames[I]))
                ++Linked;
        }
        Log("NetDelta classes loaded count %zu linked %d serializers_ready %d", ClassNames.size(), static_cast<int>(Linked), Registry.Ready() ? 1 : 0);
        return Ready();
    }

    void EntityWorld::OnCreateStringTable(const CSVCMsg_CreateStringTable& Msg)
    {
        Tables.OnCreate(Msg);
    }

    void EntityWorld::OnUpdateStringTable(const CSVCMsg_UpdateStringTable& Msg)
    {
        Tables.OnUpdate(Msg);
    }

    void EntityWorld::OnClearStringTables()
    {
        Tables.Clear();
    }

    bool EntityWorld::ApplyFields(BitReader& R, EntityState& State, bool StoreValues, std::vector<FieldPath>* PathsOut)
    {
        if (!State.Ser)
            return false;
        std::vector<FieldPath> Paths;
        if (!DecodeFieldPaths(R, Paths))
            return false;
        for (const FieldPath& Fp : Paths)
        {
            const Decoder* D = State.Ser->GetDecoder(Fp, 0);
            if (!D)
                return false;
            if (StoreValues)
            {
                FieldValue V;
                if (!ReadValue(R, *D, &V))
                    return false;
                State.Values[Fp.Key()] = std::move(V);
            }
            else if (!ReadValue(R, *D, nullptr))
            {
                return false;
            }
        }
        if (PathsOut)
            *PathsOut = std::move(Paths);
        return !R.Overflow();
    }

    bool EntityWorld::DecodeFieldsWithRanges(BitReader& R, const Serializer* Ser, Record& Rec)
    {
        Rec.Paths.clear();
        Rec.Ranges.clear();
        if (!DecodeFieldPaths(R, Rec.Paths))
            return false;
        Rec.Ranges.reserve(Rec.Paths.size());
        for (const FieldPath& Fp : Rec.Paths)
        {
            const Decoder* D = Ser->GetDecoder(Fp, 0);
            if (!D)
                return false;
            size_t Start = R.Position();
            if (!ReadValue(R, *D, nullptr))
                return false;
            Rec.Ranges.emplace_back(Start, R.Position());
        }
        return !R.Overflow();
    }

    bool EntityWorld::OnPacketEntities(CSVCMsg_PacketEntities& Msg)
    {
        ++Packets;
        if (!Ready())
        {
            if (Packets % 300 == 1)
                LogE("NetDelta not ready serializers %d class_bits %u classes %zu", Registry.Ready() ? 1 : 0, ClassBits, ClassNames.size());
            return false;
        }

        const std::string& Data = Msg.entity_data();
        const std::string& Lens = Msg.serialized_entities();
        const uint8_t* Bytes = reinterpret_cast<const uint8_t*>(Data.data());
        const uint8_t* LenBytes = reinterpret_cast<const uint8_t*>(Lens.data());

        BitReader R(Bytes, Data.size());
        size_t LenOffset = 0;
        int Count = Msg.updated_entries();
        int Index = -1;

        std::vector<Record> Records;
        Records.reserve(Count);
        PacketView View;
        View.Tick = Msg.server_tick();
        LastTick = View.Tick;

        for (int I = 0; I < Count; ++I)
        {
            Record Rec;
            Rec.HeaderStart = R.Position();
            Index += static_cast<int>(R.ReadUBitVar()) + 1;
            if (Index < 0 || Index >= MaxEntities || R.Overflow())
            {
                ++Failures;
                return false;
            }
            Rec.Index = Index;
            Rec.CmdStart = R.Position();
            Rec.Cmd = R.ReadBits(2);
            EntityState& E = Entities[Index];

            if ((Rec.Cmd & 1) == 0)
            {
                Rec.HasData = true;
                uint32_t L = 0;
                if (LenOffset < Lens.size() && ReadVarUint32Bytes(LenBytes, Lens.size(), LenOffset, L))
                {
                    Rec.HasLength = true;
                    Rec.RawLength = L;
                }

                bool Tracked;
                if (Rec.Cmd & 2)
                {
                    int ClassId = static_cast<int>(R.ReadBits(ClassBits));
                    uint32_t Serial = R.ReadBits(17);
                    R.ReadVarUint32();
                    E = EntityState{};
                    E.Active = true;
                    E.ClassId = ClassId;
                    E.Serial = Serial;
                    E.Ser = ClassSerializer(ClassId);
                    Tracked = IsTracked(E.Ser);
                    if (Tracked)
                    {
                        if (const auto* Base = Tables.Baseline(ClassId))
                        {
                            BitReader BR(Base->data(), Base->size());
                            if (!ApplyFields(BR, E, true, nullptr))
                                LogE("NetDelta baseline decode failed class %s", ClassName(ClassId).c_str());
                        }
                    }
                    View.CreatedSet[Index] = 1;
                }
                else
                {
                    if (!E.Active)
                    {
                        E.Active = true;
                    }
                    Tracked = IsTracked(E.Ser);
                }
                Rec.ClassId = E.ClassId;
                Rec.DataStart = R.Position();
                View.UpdatedSet[Index] = 1;

                bool LengthValid = Rec.HasLength && static_cast<int32_t>(Rec.RawLength) >= 0;
                bool Decoded = false;
                if (E.Ser)
                {
                    BitReader Sub(Bytes, Data.size(), Rec.DataStart,
                                  LengthValid ? Rec.DataStart + Rec.RawLength : Data.size() * 8);
                    Decoded = DecodeFieldsWithRanges(Sub, E.Ser, Rec);
                    if (Decoded && LengthValid && Sub.Position() != Rec.DataStart + Rec.RawLength)
                    {
                        Decoded = false;
                        ++Mismatches;
                        if (Mismatched < 20)
                        {
                            ++Mismatched;
                            LogE("NetDelta length mismatch class %s decoded %zu expected %u fields %zu", E.Ser->Name.c_str(),
                                 static_cast<std::size_t>(Sub.Position() - Rec.DataStart), static_cast<unsigned>(Rec.RawLength), Rec.Paths.size());
                        }
                    }
                    if (Decoded)
                    {
                        Rec.DataEnd = Sub.Position();
                        Rec.Decoded = true;
                    }
                }

                if (!Decoded)
                {
                    Rec.Paths.clear();
                    Rec.Ranges.clear();
                    if (!LengthValid)
                    {
                        ++Failures;
                        if (Failures < 20)
                            LogE("NetDelta cannot skip entity %d class %s", Index, ClassName(E.ClassId).c_str());
                        return false;
                    }
                    Rec.DataEnd = Rec.DataStart + Rec.RawLength;
                }
                else if (Tracked)
                {
                    BitReader Store(Bytes, Data.size(), Rec.DataStart, Rec.DataEnd);
                    ApplyFields(Store, E, true, nullptr);
                    View.ChangedPaths[Index] = Rec.Paths;
                }

                R.Seek(Rec.DataEnd);
            }
            else if (Rec.Cmd & 2)
            {
                E = EntityState{};
            }

            Records.push_back(std::move(Rec));
        }

        if (R.Overflow())
        {
            ++Failures;
            return false;
        }

        if (Verified < 200)
        {
            for (const Record& Rec : Records)
            {
                if (!Rec.HasData || !Rec.Decoded || !Entities[Rec.Index].Ser)
                    continue;
                const Serializer* Ser = Entities[Rec.Index].Ser;
                BitWriter W;
                if (!EncodeFieldPaths(W, Rec.Paths))
                {
                    ++VerifyFailures;
                    continue;
                }
                for (const auto& Range : Rec.Ranges)
                    W.CopyBits(Bytes, Range.first, Range.second - Range.first);
                Record Back;
                BitReader BR(W.Buffer().data(), W.Buffer().size(), 0, W.Position());
                if (!DecodeFieldsWithRanges(BR, Ser, Back) || !(Back.Paths == Rec.Paths) || BR.Position() != W.Position())
                {
                    ++VerifyFailures;
                    if (VerifyFailures < 10)
                        LogE("NetDelta path roundtrip failed class %s fields %zu", Ser->Name.c_str(), Rec.Paths.size());
                }
            }
            ++Verified;
            if (Verified == 200)
                Log("NetDelta roundtrip check done verify_failures %llu packets %llu mismatches %llu failures %llu",
                    static_cast<unsigned long long>(VerifyFailures), static_cast<unsigned long long>(Packets),
                    static_cast<unsigned long long>(Mismatches), static_cast<unsigned long long>(Failures));
        }

        if (Packets % 600 == 0)
        {
            size_t Active = 0;
            for (const auto& E : Entities)
                if (E.Active)
                    ++Active;
            Log("NetDelta stats packets %llu failures %llu mismatches %llu rewrites %llu active %llu records %zu",
                static_cast<unsigned long long>(Packets), static_cast<unsigned long long>(Failures), static_cast<unsigned long long>(Mismatches),
                static_cast<unsigned long long>(Rewrites), static_cast<unsigned long long>(Active), Records.size());
        }

        if (!Callback)
            return false;

        std::map<int, EntityEdits> Edits;
        Callback(View, Edits);
        if (Edits.empty())
            return false;

        std::string NewData;
        std::string NewLens;
        int NewCount = 0;
        if (!Encode(Msg, Records, Edits, NewData, NewLens, NewCount))
            return false;

        Msg.set_entity_data(NewData);
        Msg.set_serialized_entities(NewLens);
        Msg.set_updated_entries(NewCount);
        ++Rewrites;
        return true;
    }

    bool EntityWorld::Encode(const CSVCMsg_PacketEntities& Msg, const std::vector<Record>& Records,
                             std::map<int, EntityEdits>& Edits, std::string& OutData, std::string& OutLengths,
                             int& OutCount)
    {
        const std::string& Data = Msg.entity_data();
        const std::string& Lens = Msg.serialized_entities();
        const uint8_t* Bytes = reinterpret_cast<const uint8_t*>(Data.data());

        BitWriter W;
        std::vector<uint8_t> LenOut;
        int Prev = -1;
        OutCount = 0;

        auto WriteFields = [&](const Serializer* Ser, const Record* Rec, const EntityEdits& Ed, size_t& BitsOut) -> bool
        {
            std::map<FieldPath, Source> Merged;
            if (Rec)
            {
                for (size_t I = 0; I < Rec->Paths.size(); ++I)
                {
                    Source S;
                    S.Start = Rec->Ranges[I].first;
                    S.End = Rec->Ranges[I].second;
                    Merged[Rec->Paths[I]] = S;
                }
            }
            for (const FieldEdit& Fe : Ed.Fields)
            {
                if (Fe.Remove)
                {
                    Merged.erase(Fe.Path);
                    continue;
                }
                Source S;
                S.FromOriginal = false;
                S.Value = Fe.Value;
                Merged[Fe.Path] = S;
            }

            std::vector<FieldPath> Paths;
            Paths.reserve(Merged.size());
            for (const auto& Kv : Merged)
                Paths.push_back(Kv.first);

            size_t Start = W.Position();
            if (!EncodeFieldPaths(W, Paths))
                return false;
            for (const auto& Kv : Merged)
            {
                if (Kv.second.FromOriginal)
                {
                    W.CopyBits(Bytes, Kv.second.Start, Kv.second.End - Kv.second.Start);
                    continue;
                }
                const Decoder* D = Ser->GetDecoder(Kv.first, 0);
                if (!D || !WriteValue(W, *D, Kv.second.Value))
                    return false;
            }
            BitsOut = W.Position() - Start;
            return true;
        };

        size_t LenOffset = 0;
        auto EmitInserted = [&](int Upto) -> bool
        {
            for (auto It = Edits.begin(); It != Edits.end() && It->first < Upto;)
            {
                int Idx = It->first;
                const EntityState& E = Entities[Idx];
                bool InRecords = std::any_of(Records.begin(), Records.end(), [Idx](const Record& R) { return R.Index == Idx; });
                if (InRecords || !E.Active || !E.Ser || It->second.Fields.empty() || Idx <= Prev)
                {
                    ++It;
                    continue;
                }
                W.WriteUBitVar(static_cast<uint32_t>(Idx - Prev - 1));
                W.WriteBits(0, 2);
                size_t Bits = 0;
                if (!WriteFields(E.Ser, nullptr, It->second, Bits))
                    return false;
                AppendVarUint32(LenOut, static_cast<uint32_t>(Bits));
                Prev = Idx;
                ++OutCount;
                ++It;
            }
            return true;
        };

        for (const Record& Rec : Records)
        {
            if (!EmitInserted(Rec.Index))
                return false;

            W.WriteUBitVar(static_cast<uint32_t>(Rec.Index - Prev - 1));
            Prev = Rec.Index;
            ++OutCount;

            if (!Rec.HasData)
            {
                W.CopyBits(Bytes, Rec.CmdStart, 2);
                continue;
            }

            W.CopyBits(Bytes, Rec.CmdStart, Rec.DataStart - Rec.CmdStart);

            uint32_t OrigLen = 0;
            bool HadLen = false;
            if (Rec.HasLength)
            {
                HadLen = ReadVarUint32Bytes(reinterpret_cast<const uint8_t*>(Lens.data()), Lens.size(), LenOffset, OrigLen);
            }

            auto EdIt = Edits.find(Rec.Index);
            const EntityState& E = Entities[Rec.Index];
            bool CanEdit = EdIt != Edits.end() && !EdIt->second.Fields.empty() && Rec.Decoded && E.Ser;

            if (CanEdit)
            {
                size_t Bits = 0;
                if (!WriteFields(E.Ser, &Rec, EdIt->second, Bits))
                    return false;
                if (HadLen)
                    AppendVarUint32(LenOut, static_cast<uint32_t>(Bits));
            }
            else
            {
                W.CopyBits(Bytes, Rec.DataStart, Rec.DataEnd - Rec.DataStart);
                if (HadLen)
                    AppendVarUint32(LenOut, OrigLen);
            }
        }

        if (!EmitInserted(MaxEntities))
            return false;

        size_t TailStart = Records.empty() ? 0 : Records.back().DataEnd;
        if (!Records.empty() && !Records.back().HasData)
            TailStart = Records.back().CmdStart + 2;
        size_t TotalBits = Data.size() * 8;
        if (TailStart < TotalBits)
        {
            size_t TailBits = TotalBits - TailStart;
            bool AllZero = true;
            for (size_t B = TailStart; B < TotalBits; ++B)
                if ((Bytes[B >> 3] >> (B & 7)) & 1)
                {
                    AllZero = false;
                    break;
                }
            if (!AllZero)
                W.CopyBits(Bytes, TailStart, TailBits);
        }

        if (LenOffset < Lens.size())
            LenOut.insert(LenOut.end(), Lens.begin() + LenOffset, Lens.end());

        OutData.assign(reinterpret_cast<const char*>(W.Buffer().data()), W.Buffer().size());
        OutLengths.assign(reinterpret_cast<const char*>(LenOut.data()), LenOut.size());
        return true;
    }
}
