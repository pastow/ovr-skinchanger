#include "CNetworkGameClient.h"

#include "Misc/Offsets.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"

#include <windows.h>

#include <cstdint>

namespace
{
    constexpr const char* ClientServiceName = "NetworkClientService_001";
    constexpr const char* ServerServiceName = "NetworkServerService_001";

    void* ClientService = nullptr;
    void* ServerService = nullptr;

    template <typename Fn>
    Fn Virtual(const void* Object, std::size_t Index)
    {
        return reinterpret_cast<Fn>(Memory::Read<void**>(reinterpret_cast<std::uintptr_t>(Object))[Index]);
    }

    std::uintptr_t Self(const void* Object)
    {
        return reinterpret_cast<std::uintptr_t>(Object);
    }
}

bool CNetworkGameClient::Init()
{
    HMODULE Module = GetModuleHandleA("engine2.dll");
    auto CreateInterface = Module ? reinterpret_cast<void* (*)(const char*, int*)>(GetProcAddress(Module, "CreateInterface")) : nullptr;
    if (!CreateInterface)
    {
        LogE("engine2.dll CreateInterface not found");
        return false;
    }
    ClientService = CreateInterface(ClientServiceName, nullptr);
    ServerService = CreateInterface(ServerServiceName, nullptr);
    if (!ClientService || !ServerService)
    {
        LogE("Network services not found client %p server %p", ClientService, ServerService);
        return false;
    }
    Log("%s %p %s %p", ClientServiceName, ClientService, ServerServiceName, ServerService);
    return true;
}

CNetworkGameClient* CNetworkGameClient::Get()
{
    return ClientService ? Virtual<CNetworkGameClient* (*)(void*)>(ClientService, Offsets::CNetworkClientService::GameClientIndex)(ClientService) : nullptr;
}

int CNetworkGameClient::ClassCount() const
{
    return Memory::Read<int>(Self(this) + Offsets::CNetworkGameClient::ClassCount);
}

const char* CNetworkGameClient::ClassName(int ClassId) const
{
    if (ClassId < 0 || ClassId >= ClassCount())
        return nullptr;
    std::uintptr_t Classes = Memory::Read<std::uintptr_t>(Self(this) + Offsets::CNetworkGameClient::Classes);
    return Classes ? Memory::Read<const char*>(Classes + Offsets::CNetworkGameClient::ClassEntrySize * ClassId) : nullptr;
}

CProtobufMessage* CNetworkGameClient::Serializers() const
{
    std::uintptr_t Holder = Memory::Read<std::uintptr_t>(Self(this) + Offsets::CNetworkGameClient::SerializerCache);
    if (!Holder && ServerService)
        Holder = reinterpret_cast<std::uintptr_t>(
            Virtual<void* (*)(void*)>(ServerService, Offsets::CNetworkServerService::SerializersIndex)(ServerService));
    return Holder ? reinterpret_cast<CProtobufMessage*>(Holder + Offsets::CNetworkGameClient::SerializerProto) : nullptr;
}
