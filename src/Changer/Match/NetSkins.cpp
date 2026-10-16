#include "NetSkins.h"

#include "Changer/Items/AssetPrecache.h"
#include "Changer/Items/FakeLoadout.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTAPlayerController.h"
#include "SDK/EntitySystem/EntityUtils/C_DOTA_BaseNPC_Hero.h"
#include "SDK/Logger/Logger.h"
#include "SDK/NetDelta/EntityWorld.h"
#include "SDK/NetDelta/ResourceId.h"
#include "SDK/ResourceSystem/CModel.h"
#include "SDK/ResourceSystem/CResourceSystem.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr const char* HeroClassPrefix = "CDOTA_Unit_Hero_";
    constexpr const char* EntityModelType = "entity_model";
    constexpr const char* ModelField = "CBodyComponent.m_hModel";
    constexpr const char* MaterialGroupField = "CBodyComponent.m_materialGroup";
    constexpr const char* MeshGroupMaskField = "CBodyComponent.m_MeshGroupMask";
    constexpr const char* PlayerIdField = "m_iPlayerID";
    constexpr const char* SequenceFieldName = "m_hSequence";
    constexpr const char* PoseFieldName = "m_flPoseParameter";
    constexpr int MaxFieldDepth = 4;

    struct WantState
    {
        int Index = -1;
        std::uint64_t Model = 0;
        std::uint64_t MaterialGroup = 0;
        std::string Path;
        std::uint64_t ClientModel = 0;
        std::uint32_t ObservedTick = 0;
    };

    std::mutex WantMutex;
    WantState Want;
    bool Installed = false;

    std::uint32_t PrecachedConnection = 0;
    std::uint64_t LastClientModel = 0;
    std::string LastModelReport;
    std::unordered_map<int, std::uint64_t> DefaultModel;
    std::unordered_map<int, std::uint64_t> ClientModel;
    std::unordered_map<int, std::uint32_t> LastEditTick;
    std::mutex SwappedMutex;
    std::unordered_map<int, std::uint64_t> Swapped;
    std::unordered_map<int, std::uint64_t> SwappedDefault;

    void MarkSwapped(int Index, std::uint64_t Model, std::uint64_t Default)
    {
        std::lock_guard<std::mutex> Lock(SwappedMutex);
        if (Model)
        {
            Swapped[Index] = Model;
            SwappedDefault[Index] = Default;
        }
        else
        {
            Swapped.erase(Index);
            SwappedDefault.erase(Index);
        }
    }

    std::string LastAnimationReport;

    NetDelta::FieldValue MakeValue(std::uint64_t Value)
    {
        NetDelta::FieldValue Out;
        Out.U = Value;
        Out.I = static_cast<std::int64_t>(Value);
        return Out;
    }

    void AddEdit(const NetDelta::EntityState& Entity, const char* Name, std::uint64_t Value, NetDelta::EntityEdits& Edits)
    {
        NetDelta::FieldPath Path;
        if (!Entity.Ser->FindPath(Name, Path))
            return;
        Edits.Fields.push_back({Path, MakeValue(Value), false});
    }

    std::string FindField(const NetDelta::Serializer* Ser, const char* Name, int Depth)
    {
        if (!Ser || Depth > MaxFieldDepth)
            return std::string();
        for (const NetDelta::Field* Field : Ser->Fields)
        {
            if (Field->Name == Name)
                return Field->Name;
            if (Field->Model != NetDelta::FieldModel::FixedTable || !Field->Child)
                continue;
            std::string Inner = FindField(Field->Child, Name, Depth + 1);
            if (!Inner.empty())
                return Field->Name + "." + Inner;
        }
        return std::string();
    }

    bool IsElementOf(const NetDelta::FieldPath& Path, const NetDelta::FieldPath& Root)
    {
        if (Path.Last != Root.Last + 1)
            return false;
        for (int Position = 0; Position <= Root.Last; ++Position)
            if (Path.Path[Position] != Root.Path[Position])
                return false;
        return true;
    }

    void TranslateAnimation(const NetDelta::PacketView& View, int Index, const NetDelta::EntityState& Entity, std::uint64_t FromId,
                            std::uint64_t ToId, NetDelta::EntityEdits& Edit)
    {
        auto Changed = View.Changed().find(Index);
        CResourceSystem* Resources = CResourceSystem::Get();
        if (Changed == View.Changed().end() || !Resources)
            return;

        auto* From = static_cast<const CModel*>(Resources->Data(FromId));
        auto* To = static_cast<const CModel*>(Resources->Data(ToId));
        if (!From || !To)
            return;

        std::string Sequence = FindField(Entity.Ser, SequenceFieldName, 0);
        std::string Pose = FindField(Entity.Ser, PoseFieldName, 0);
        std::string Report = (Sequence.empty() ? std::string("?") : Sequence) + " " + (Pose.empty() ? std::string("?") : Pose);
        if (Report != LastAnimationReport)
        {
            Log("NetSkins animation fields %s", Report.c_str());
            LastAnimationReport = Report;
        }

        NetDelta::FieldPath SequencePath;
        NetDelta::FieldPath PoseRoot;
        bool HasSequence = !Sequence.empty() && Entity.Ser->FindPath(Sequence, SequencePath);
        bool HasPose = !Pose.empty() && Entity.Ser->FindPath(Pose, PoseRoot);

        std::vector<NetDelta::FieldEdit> Sets;
        for (const NetDelta::FieldPath& Path : Changed->second)
        {
            if (HasSequence && Path == SequencePath)
            {
                const NetDelta::FieldValue* Server = Entity.Get(Sequence);
                if (!Server)
                    continue;
                int ServerSequence = static_cast<std::int32_t>(Server->U);
                int Mapped = To->LookupSequence(From->SequenceName(ServerSequence));
                if (Mapped < 0 || Mapped == ServerSequence)
                    continue;
                NetDelta::FieldValue Value = *Server;
                Value.I = Mapped;
                Value.U = static_cast<std::uint64_t>(Mapped);
                Sets.push_back({Path, Value, false});
                continue;
            }

            if (!HasPose || !IsElementOf(Path, PoseRoot))
                continue;
            int Slot = Path.Path[Path.Last];
            int Mapped = To->LookupPoseParameter(From->PoseParameterName(Slot));
            if (Mapped == Slot)
                continue;
            Edit.Fields.push_back({Path, NetDelta::FieldValue(), true});
            const NetDelta::FieldValue* Server = Entity.Get(Pose + "." + std::to_string(Slot));
            if (!Server || Mapped < 0)
                continue;
            NetDelta::FieldPath Target = Path;
            Target.Path[Target.Last] = Mapped;
            Sets.push_back({Target, *Server, false});
        }

        for (const NetDelta::FieldEdit& Set : Sets)
            Edit.Fields.push_back(Set);
    }

    void ProcessHero(const NetDelta::PacketView& View, std::map<int, NetDelta::EntityEdits>& Edits, const WantState& State, int Index,
                     const NetDelta::EntityState& Entity, bool IsLocal)
    {
        NetDelta::FieldPath ModelPath;
        if (!Entity.Ser->FindPath(ModelField, ModelPath))
            return;
        const NetDelta::FieldValue* Server = Entity.Get(ModelField);
        if (!Server)
            return;

        if (!DefaultModel.count(Index))
            DefaultModel[Index] = Server->U;

        auto Changed = View.Changed().find(Index);
        if (Changed != View.Changed().end())
            for (const auto& Path : Changed->second)
                if (Path == ModelPath)
                    ClientModel[Index] = Server->U;
        if (!ClientModel.count(Index))
            ClientModel[Index] = Server->U;

        bool UseWant = State.Model && Server->U == DefaultModel[Index];
        std::uint64_t Desired = UseWant ? State.Model : Server->U;
        if (UseWant)
            TranslateAnimation(View, Index, Entity, Server->U, State.Model, Edits[Index]);
        bool Resend = IsLocal && UseWant && ClientModel[Index] == Desired && State.ClientModel && State.ClientModel != State.Model &&
                      State.ObservedTick > LastEditTick[Index];
        if (ClientModel[Index] == Desired && !Resend)
            return;
        if (Resend)
            Log("NetSkins hero %d client model %#llx after tick %u, resending", Index, static_cast<unsigned long long>(State.ClientModel),
                LastEditTick[Index]);

        auto& Edit = Edits[Index];
        AddEdit(Entity, ModelField, Desired, Edit);
        if (UseWant)
        {
            AddEdit(Entity, MaterialGroupField, State.MaterialGroup, Edit);
            AddEdit(Entity, MeshGroupMaskField, ~0ull, Edit);
        }
        else
        {
            AddEdit(Entity, MaterialGroupField, Entity.GetUint(MaterialGroupField), Edit);
            AddEdit(Entity, MeshGroupMaskField, Entity.GetUint(MeshGroupMaskField), Edit);
        }
        ClientModel[Index] = Desired;
        LastEditTick[Index] = View.Tick;
        MarkSwapped(Index, UseWant ? Desired : 0, DefaultModel[Index]);
        Log("NetSkins %s %d model -> %s %#llx tick %u", IsLocal ? "hero" : "illusion", Index, UseWant ? State.Path.c_str() : "server",
            static_cast<unsigned long long>(Desired), View.Tick);
    }

    void Edit(const NetDelta::PacketView& View, std::map<int, NetDelta::EntityEdits>& Edits)
    {
        WantState State;
        {
            std::lock_guard<std::mutex> Lock(WantMutex);
            State = Want;
        }

        auto& World = NetDelta::EntityWorld::Instance();
        if (PrecachedConnection != World.Connection())
        {
            PrecachedConnection = World.Connection();
            AssetPrecache::LoadEquipped();
        }
        for (const auto& [Index, Unused] : View.CreatedSet)
        {
            DefaultModel.erase(Index);
            ClientModel.erase(Index);
            LastEditTick.erase(Index);
            MarkSwapped(Index, 0, 0);
        }

        if (State.Index < 0)
            return;
        const NetDelta::EntityState* Local = World.Get(State.Index);
        if (!Local || !Local->Ser)
            return;
        const NetDelta::FieldValue* LocalPlayer = Local->Get(PlayerIdField);

        ProcessHero(View, Edits, State, State.Index, *Local, true);
        if (!LocalPlayer)
            return;
        for (int Index = 0; Index < NetDelta::MaxEntities; ++Index)
        {
            if (Index == State.Index)
                continue;
            const NetDelta::EntityState* Entity = World.Get(Index);
            if (!Entity || !Entity->Ser || Entity->Ser->Name.rfind(HeroClassPrefix, 0) != 0)
                continue;
            const NetDelta::FieldValue* Player = Entity->Get(PlayerIdField);
            if (!Player || Player->U != LocalPlayer->U)
                continue;
            ProcessHero(View, Edits, State, Index, *Entity, false);
        }
    }

    void Install()
    {
        auto& World = NetDelta::EntityWorld::Instance();
        World.SetTracked([](const std::string& Name) { return Name.rfind(HeroClassPrefix, 0) == 0; });
        World.SetEditCallback(Edit);
        Installed = true;
    }
}

void NetSkins::Update()
{
    if (!Installed)
        Install();

    WantState State;
    C_DOTA_BaseNPC_Hero* Hero = C_DOTAPlayerController::LocalHero();
    if (Hero)
    {
        State.Index = Hero->Index();
        State.ClientModel = Hero->ModelId();
        State.ObservedTick = NetDelta::EntityWorld::Instance().ServerTick();
        if (State.ClientModel != LastClientModel)
        {
            Log("NetSkins client model %d %#llx -> %#llx", State.Index, static_cast<unsigned long long>(LastClientModel),
                static_cast<unsigned long long>(State.ClientModel));
            LastClientModel = State.ClientModel;
        }

        const char* UnitName = Hero->DesignerName();
        FakeLoadout::Source Source = UnitName ? FakeLoadout::FindSource(EntityModelType, UnitName) : FakeLoadout::Source();
        std::string Path = Source.Modifier;
        std::string Report = std::string(UnitName ? UnitName : "?") + " " + (Path.empty() ? std::string("no entity_model") : Path) + " style " +
                             std::to_string(Source.Style) + " skin " + std::to_string(Source.Definition ? Source.Definition->StyleSkin(Source.Style) : 0);
        if (Report != LastModelReport)
        {
            Log("NetSkins hero %d %s", State.Index, Report.c_str());
            LastModelReport = Report;
        }
        if (!Path.empty())
        {
            State.Path = Path;
            std::uint64_t Id = NetDelta::ResourceId(Path.c_str());
            CResourceSystem* Resources = CResourceSystem::Get();
            if (Resources && Resources->Status(Id) == CResourceSystem::EStatus::Loaded)
            {
                State.Model = Id;
                auto* Model = static_cast<CModel*>(Resources->Data(Id));
                int Skin = Source.Definition ? Source.Definition->StyleSkin(Source.Style) : 0;
                State.MaterialGroup = Model ? Model->MaterialGroup(Skin) : 0;
            }
            else
                AssetPrecache::Ensure(Path);
        }
    }

    std::lock_guard<std::mutex> Lock(WantMutex);
    Want = std::move(State);
}

int NetSkins::LocalHeroIndex()
{
    std::lock_guard<std::mutex> Lock(WantMutex);
    return Want.Index;
}


bool NetSkins::IsSwapped(int Index, std::uint64_t ModelId)
{
    std::lock_guard<std::mutex> Lock(SwappedMutex);
    auto It = Swapped.find(Index);
    return It != Swapped.end() && ModelId && It->second == ModelId;
}

bool NetSkins::IsOwnHero(int Index)
{
    if (Index < 0)
        return false;
    if (Index == LocalHeroIndex())
        return true;
    std::lock_guard<std::mutex> Lock(SwappedMutex);
    return Swapped.count(Index) != 0;
}

std::uint64_t NetSkins::DefaultModelOf(int Index)
{
    std::lock_guard<std::mutex> Lock(SwappedMutex);
    auto It = SwappedDefault.find(Index);
    return It != SwappedDefault.end() ? It->second : 0;
}
