#include "PacketRewriter.h"

#include "Changer/Animation/AnimationFix.h"
#include "Changer/Animation/AnimationLogger.h"
#include "Changer/Match/EmblemEffect.h"
#include "Changer/Match/ImpactEffects.h"
#include "SDK/Files/Paths.h"
#include "SDK/Logger/Logger.h"
#include "SDK/NetDelta/EntityWorld.h"
#include "SDK/Network/CNetMessage.h"
#include "SDK/Network/CNetworkGameClient.h"
#include "SDK/Protobuf/CProtobufMessage.h"

#include "netmessages.pb.h"

#include <cstdio>
#include <string>
#include <vector>

namespace
{
    constexpr std::uint32_t SvcServerInfo = 40;
    constexpr std::uint32_t SvcFlattenedSerializer = 41;
    constexpr std::uint32_t SvcClassInfo = 42;
    constexpr std::uint32_t SvcCreateStringTable = 44;
    constexpr std::uint32_t SvcUpdateStringTable = 45;
    constexpr std::uint32_t SvcClearAllStringTables = 51;
    constexpr std::uint32_t SvcPacketEntities = 55;
    constexpr std::uint32_t DotaTeProjectile = 518;
    constexpr std::uint32_t DotaTeProjectileLoc = 519;
    constexpr std::uint32_t DotaTeUnitAnimation = 521;
    constexpr std::uint32_t UmParticleManager = 145;
    constexpr std::uint32_t DotaTeDestroyProjectile = 571;
    constexpr int DumpPacketLimit = 3000;
    constexpr const char* DumpFileName = "netdelta_dump.bin";

    int DumpedPackets = 0;

    void DumpRecord(std::uint8_t Type, const std::string& Bytes)
    {
        FILE* File = std::fopen(Paths::Resolve(DumpFileName).c_str(), "ab");
        if (!File)
            return;
        std::uint32_t Length = static_cast<std::uint32_t>(Bytes.size());
        std::fwrite(&Type, 1, 1, File);
        std::fwrite(&Length, 4, 1, File);
        std::fwrite(Bytes.data(), 1, Bytes.size(), File);
        std::fclose(File);
    }

    void LoadClasses(NetDelta::EntityWorld& World)
    {
        CNetworkGameClient* Client = CNetworkGameClient::Get();
        int Count = Client ? Client->ClassCount() : 0;
        CProtobufMessage* Proto = Client ? Client->Serializers() : nullptr;
        if (Count <= 0 || !Proto)
            return;

        CSVCMsg_FlattenedSerializer Serializers;
        if (!Proto->CopyTo(Serializers))
        {
            LogE("NetDelta serializers copy failed");
            return;
        }

        std::vector<std::string> Names(Count);
        std::string Joined;
        for (int Index = 0; Index < Count; ++Index)
        {
            if (const char* Name = Client->ClassName(Index))
                Names[Index] = Name;
            Joined += Names[Index];
            Joined += '\n';
        }
        DumpRecord(1, Serializers.SerializeAsString());
        DumpRecord(2, Joined);
        World.LoadClasses(Names, Serializers);
    }

    template <typename T>
    bool Copy(CNetMessage* Message, T& Out)
    {
        CProtobufMessage* Proto = Message->Proto();
        return Proto && Proto->CopyTo(Out);
    }
}

void PacketRewriter::OnMessage(CNetMessage* Message)
{
    if (!Message)
        return;

    auto& World = NetDelta::EntityWorld::Instance();
    switch (Message->Id())
    {
    case SvcPacketEntities:
    {
        if (!World.ClassesLoaded())
            LoadClasses(World);
        CSVCMsg_PacketEntities Packet;
        if (!Copy(Message, Packet))
            return;
        if (World.ClassesLoaded() && DumpedPackets < DumpPacketLimit)
        {
            ++DumpedPackets;
            DumpRecord(6, Packet.SerializeAsString());
        }
        if (World.OnPacketEntities(Packet) && !Message->Proto()->CopyFrom(Packet))
            LogE("NetDelta packet write back failed tick %u", World.ServerTick());
        return;
    }
    case SvcFlattenedSerializer:
    {
        CSVCMsg_FlattenedSerializer Serializers;
        if (Copy(Message, Serializers))
            World.OnSerializers(Serializers);
        return;
    }
    case SvcServerInfo:
    {
        CSVCMsg_ServerInfo Info;
        if (!Copy(Message, Info))
            return;
        DumpRecord(3, Info.SerializeAsString());
        World.OnServerInfo(Info);
        return;
    }
    case SvcClassInfo:
    {
        CSVCMsg_ClassInfo Info;
        if (Copy(Message, Info))
            World.OnClassInfo(Info);
        return;
    }
    case SvcCreateStringTable:
    {
        CSVCMsg_CreateStringTable Table;
        if (!Copy(Message, Table))
            return;
        DumpRecord(4, Table.SerializeAsString());
        World.OnCreateStringTable(Table);
        return;
    }
    case SvcUpdateStringTable:
    {
        CSVCMsg_UpdateStringTable Table;
        if (!Copy(Message, Table))
            return;
        DumpRecord(5, Table.SerializeAsString());
        World.OnUpdateStringTable(Table);
        return;
    }
    case DotaTeProjectile:
        AnimationLogger::OnProjectile(Message, false);
        EmblemEffect::OnProjectile(Message, false);
        return;
    case DotaTeProjectileLoc:
        AnimationLogger::OnProjectile(Message, true);
        EmblemEffect::OnProjectile(Message, true);
        return;
    case DotaTeUnitAnimation:
        AnimationLogger::OnUnitAnimation(Message);
        AnimationFix::OnUnitAnimation(Message);
        return;
    case DotaTeDestroyProjectile:
        ImpactEffects::OnDestroyProjectile(Message);
        return;
    case UmParticleManager:
        AnimationLogger::OnParticleManager(Message);
        return;
    case SvcClearAllStringTables:
        DumpRecord(7, std::string());
        World.OnClearStringTables();
        return;
    default:
        return;
    }
}
