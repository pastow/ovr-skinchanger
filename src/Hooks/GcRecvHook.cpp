#include "GcRecvHook.h"

#include "Changer/Inventory/InventorySync.h"
#include "Misc/Patterns.h"
#include "SDK/GC/CGCClient.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include "gcsdk_gcmessages.pb.h"

#include <MinHook.h>

#include <atomic>
#include <cstring>
#include <deque>
#include <mutex>

namespace
{
    constexpr std::size_t SlotIsMessageAvailable = 1;
    constexpr std::size_t SlotRetrieveMessage = 2;
    constexpr int ResultOk = 0;
    constexpr int ResultBufferTooSmall = 2;

    void (*Original)(void*) = nullptr;
    std::atomic<bool> PumpRequested{false};

    VmtHook AvailableHook;
    VmtHook RetrieveHook;

    std::mutex                            FakeMutex;
    std::deque<std::vector<std::uint8_t>> FakeQueue;

    std::mutex          OwnerMutex;
    GcRecvHook::SoOwner CurrentOwner;

    void CaptureOwner(std::uint32_t EMsg, const std::string& Body)
    {
        CMsgSOIDOwner Owner;
        std::uint64_t Version = 0;
        if (EMsg == 24)
        {
            CMsgSOCacheSubscribed Msg;
            if (!Msg.ParseFromString(Body))
                return;
            Owner = Msg.owner_soid();
            Version = Msg.version();
        }
        else if (EMsg == 29)
        {
            CMsgSOCacheSubscribedUpToDate Msg;
            if (!Msg.ParseFromString(Body))
                return;
            Owner = Msg.owner_soid();
            Version = Msg.version();
        }
        else if (EMsg == 26)
        {
            CMsgSOMultipleObjects Msg;
            if (!Msg.ParseFromString(Body))
                return;
            Owner = Msg.owner_soid();
            Version = Msg.version();
        }
        else
        {
            return;
        }
        if (Owner.type() != 1)
            return;
        std::lock_guard<std::mutex> Lock(OwnerMutex);
        CurrentOwner = {Owner.id(), Version, true};
        if (EMsg == 24)
            Log("SO cache subscribed owner %llu version %llu", static_cast<unsigned long long>(CurrentOwner.Id),
                static_cast<unsigned long long>(CurrentOwner.Version));
    }

    void Process(std::vector<std::uint8_t>& Message)
    {
        if (Message.size() < 8)
            return;
        std::uint32_t Type = 0;
        std::uint32_t HeaderLen = 0;
        std::memcpy(&Type, Message.data(), 4);
        std::memcpy(&HeaderLen, Message.data() + 4, 4);
        std::size_t Offset = 8 + static_cast<std::size_t>(HeaderLen);
        if (!(Type & 0x80000000u) || Offset > Message.size())
            return;

        std::uint32_t EMsg = Type & 0x7FFFFFFFu;
        std::string Body(reinterpret_cast<const char*>(Message.data() + Offset), Message.size() - Offset);
        CaptureOwner(EMsg, Body);
        if (!InventorySync::Rewrite(EMsg, Body))
            return;
        Message.resize(Offset + Body.size());
        std::memcpy(Message.data() + Offset, Body.data(), Body.size());
    }

    bool PullReal(void* Gc)
    {
        std::uint32_t Size = 0;
        if (!reinterpret_cast<bool (*)(void*, std::uint32_t*)>(AvailableHook.Original())(Gc, &Size))
            return false;

        auto Retrieve = reinterpret_cast<int (*)(void*, std::uint32_t*, void*, std::uint32_t, std::uint32_t*)>(RetrieveHook.Original());
        std::vector<std::uint8_t> Message(Size);
        std::uint32_t Type = 0;
        std::uint32_t Received = 0;
        int Result = Retrieve(Gc, &Type, Message.data(), Size, &Received);
        if (Result == ResultBufferTooSmall)
        {
            Message.resize(Received);
            Result = Retrieve(Gc, &Type, Message.data(), Received, &Received);
        }
        if (Result != ResultOk)
            return false;
        Message.resize(Received);
        Process(Message);

        std::lock_guard<std::mutex> Lock(FakeMutex);
        FakeQueue.push_front(std::move(Message));
        return true;
    }

    bool IsMessageAvailable(void* Gc, std::uint32_t* Size)
    {
        {
            std::lock_guard<std::mutex> Lock(FakeMutex);
            if (!FakeQueue.empty())
            {
                if (Size)
                    *Size = static_cast<std::uint32_t>(FakeQueue.front().size());
                return true;
            }
        }
        if (!PullReal(Gc))
            return false;
        std::lock_guard<std::mutex> Lock(FakeMutex);
        if (Size)
            *Size = static_cast<std::uint32_t>(FakeQueue.front().size());
        return true;
    }

    int RetrieveMessage(void* Gc, std::uint32_t* Type, void* Dest, std::uint32_t DestSize, std::uint32_t* Size)
    {
        {
            std::lock_guard<std::mutex> Lock(FakeMutex);
            if (!FakeQueue.empty())
            {
                const auto& Message = FakeQueue.front();
                std::uint32_t Need = static_cast<std::uint32_t>(Message.size());
                if (Size)
                    *Size = Need;
                if (!Dest || Need > DestSize)
                    return ResultBufferTooSmall;
                std::memcpy(Dest, Message.data(), Need);
                if (Type)
                    std::memcpy(Type, Message.data(), 4);
                FakeQueue.pop_front();
                return ResultOk;
            }
        }
        if (PullReal(Gc))
            return RetrieveMessage(Gc, Type, Dest, DestSize, Size);
        return reinterpret_cast<int (*)(void*, std::uint32_t*, void*, std::uint32_t, std::uint32_t*)>(RetrieveHook.Original())(
            Gc, Type, Dest, DestSize, Size);
    }

    void HookSteamGc(CGCClient* Client)
    {
        if (RetrieveHook.IsInstalled() || !Client)
            return;
        void* Gc = Client->SteamGameCoordinator();
        if (Gc && AvailableHook.Install(Gc, SlotIsMessageAvailable, reinterpret_cast<void*>(IsMessageAvailable)) &&
            RetrieveHook.Install(Gc, SlotRetrieveMessage, reinterpret_cast<void*>(RetrieveMessage)))
            Log("Steam GC hooked %p", Gc);
    }

    void Detour(void* Client)
    {
        HookSteamGc(static_cast<CGCClient*>(Client));

        Original(Client);
    }
}

void GcRecvHook::PushFake(std::vector<std::uint8_t> Message)
{
    std::lock_guard<std::mutex> Lock(FakeMutex);
    FakeQueue.push_back(std::move(Message));
}

void GcRecvHook::Pump()
{
    PumpRequested.store(true);
}

void GcRecvHook::Update()
{
    if (!Original || !PumpRequested.exchange(false))
        return;
    if (CGCClient* Client = CGCClient::Get())
        Detour(Client);
}

GcRecvHook::SoOwner GcRecvHook::Owner()
{
    std::lock_guard<std::mutex> Lock(OwnerMutex);
    return CurrentOwner;
}

bool GcRecvHook::Install()
{
    std::uintptr_t Target = Memory::FindPattern("client.dll", Patterns::GcRecvPump);
    if (!Target)
    {
        LogE("GcRecvPump pattern not found");
        return false;
    }

    MH_STATUS Status = MH_Initialize();
    if (Status != MH_OK && Status != MH_ERROR_ALREADY_INITIALIZED)
    {
        LogE("MinHook init failed: %s", MH_StatusToString(Status));
        return false;
    }

    auto* TargetFn = reinterpret_cast<void*>(Target);
    Status = MH_CreateHook(TargetFn, reinterpret_cast<void*>(Detour), reinterpret_cast<void**>(&Original));
    if (Status == MH_OK)
        Status = MH_EnableHook(TargetFn);

    if (Status != MH_OK)
    {
        LogE("GcRecvPump hook failed: %s", MH_StatusToString(Status));
        return false;
    }

    Log("GcRecvPump %#llx", static_cast<unsigned long long>(Target));
    HookSteamGc(CGCClient::Get());
    return true;
}
