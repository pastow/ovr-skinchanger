#pragma once
#include "FieldPath.h"
#include "Serializers.h"
#include "StringTables.h"
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

class CSVCMsg_PacketEntities;
class CSVCMsg_ClassInfo;
class CSVCMsg_ServerInfo;
class CSVCMsg_FlattenedSerializer;
class CSVCMsg_CreateStringTable;
class CSVCMsg_UpdateStringTable;

namespace NetDelta
{
    constexpr int MaxEntities = 16384;

    struct EntityState
    {
        bool Active = false;
        int  ClassId = -1;
        uint32_t Serial = 0;
        const Serializer* Ser = nullptr;
        std::unordered_map<std::string, FieldValue> Values;

        const FieldValue* Get(const std::string& Dotted) const;
        uint64_t GetUint(const std::string& Dotted, uint64_t Default = 0) const;
        float    GetFloat(const std::string& Dotted, float Default = 0.0f) const;
        uint32_t Handle(int Index) const { return (Serial << 14) | static_cast<uint32_t>(Index); }
    };

    struct FieldEdit
    {
        FieldPath  Path;
        FieldValue Value;
        bool       Remove = false;
    };

    struct EntityEdits
    {
        std::vector<FieldEdit> Fields;
    };

    class PacketView
    {
    public:
        bool Updated(int Index) const { return UpdatedSet.count(Index) != 0; }
        bool Created(int Index) const { return CreatedSet.count(Index) != 0; }
        const std::unordered_map<int, std::vector<FieldPath>>& Changed() const { return ChangedPaths; }
        uint32_t Tick = 0;

        std::unordered_map<int, uint8_t> UpdatedSet;
        std::unordered_map<int, uint8_t> CreatedSet;
        std::unordered_map<int, std::vector<FieldPath>> ChangedPaths;
    };

    using EditCallback = std::function<void(const PacketView&, std::map<int, EntityEdits>&)>;

    class EntityWorld
    {
    public:
        static EntityWorld& Instance();

        void OnServerInfo(const CSVCMsg_ServerInfo& Msg);
        void OnClassInfo(const CSVCMsg_ClassInfo& Msg);
        void OnSerializers(const CSVCMsg_FlattenedSerializer& Msg);
        bool LoadClasses(const std::vector<std::string>& Names, const CSVCMsg_FlattenedSerializer& Msg);
        void OnCreateStringTable(const CSVCMsg_CreateStringTable& Msg);
        void OnUpdateStringTable(const CSVCMsg_UpdateStringTable& Msg);
        void OnClearStringTables();

        bool OnPacketEntities(CSVCMsg_PacketEntities& Msg);

        void SetEditCallback(EditCallback Cb) { Callback = std::move(Cb); }
        void SetTracked(std::function<bool(const std::string&)> Pred) { TrackPred = std::move(Pred); }

        bool Ready() const;
        bool ClassesLoaded() const { return !ClassNames.empty(); }
        const EntityState* Get(int Index) const;
        EntityState* GetMutable(int Index);
        const Serializer* ClassSerializer(int ClassId) const;
        std::string ClassName(int ClassId) const;
        int  ClassIdByName(const std::string& Name) const;
        uint32_t ServerTick() const { return LastTick; }
        uint32_t Connection() const { return Connections; }
        float TickInterval() const { return Interval; }

        bool ApplyFields(BitReader& R, EntityState& State, bool StoreValues, std::vector<FieldPath>* PathsOut);

    private:
        struct Record
        {
            int      Index = 0;
            uint32_t Cmd = 0;
            size_t   HeaderStart = 0;
            size_t   CmdStart = 0;
            size_t   DataStart = 0;
            size_t   DataEnd = 0;
            bool     HasData = false;
            bool     Decoded = false;
            bool     HasLength = false;
            uint32_t RawLength = 0;
            int      ClassId = -1;
            std::vector<FieldPath> Paths;
            std::vector<std::pair<size_t, size_t>> Ranges;
        };

        void RebuildClassMap();
        bool DecodeFieldsWithRanges(BitReader& R, const Serializer* Ser, Record& Rec);
        bool Encode(const CSVCMsg_PacketEntities& Msg, const std::vector<Record>& Records,
                    std::map<int, EntityEdits>& Edits, std::string& OutData, std::string& OutLengths,
                    int& OutCount);
        bool IsTracked(const Serializer* Ser) const;

        SerializerRegistry Registry;
        StringTables Tables;
        std::vector<EntityState> Entities = std::vector<EntityState>(MaxEntities);
        std::vector<std::string> ClassNames;
        std::unordered_map<std::string, int> ClassIds;
        bool CreateOnClient = false;
        bool ClassInfoReceived = false;
        int  MaxClasses = 0;
        uint32_t ClassBits = 0;
        uint32_t LastTick = 0;
        float Interval = 1.0f / 30.0f;
        EditCallback Callback;
        std::function<bool(const std::string&)> TrackPred;
        uint32_t Connections = 0;
        uint64_t Packets = 0;
        uint64_t Failures = 0;
        uint64_t Rewrites = 0;
        uint64_t Mismatches = 0;
        bool RoundTripChecked = false;
    };
}
