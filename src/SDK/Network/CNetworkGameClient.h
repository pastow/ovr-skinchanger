#pragma once

#include <cstdint>

class CProtobufMessage;

class CNetworkGameClient
{
public:
    static bool Init();
    static CNetworkGameClient* Get();

    int ClassCount() const;
    const char* ClassName(int ClassId) const;
    CProtobufMessage* Serializers() const;
};
