#pragma once

#include <cstdint>

class CProtobufMessage;

class CNetMessage
{
public:
    std::uint32_t Id() const;
    CProtobufMessage* Proto() const;
};
