#pragma once

#include <string>

class CProtobufMessage
{
public:
    bool SerializeTo(std::string& Out) const;
    bool ParseFrom(const std::string& Data);

    template <typename T>
    bool CopyTo(T& Out) const
    {
        std::string Bytes;
        return SerializeTo(Bytes) && Out.ParsePartialFromString(Bytes);
    }

    template <typename T>
    bool CopyFrom(const T& Message)
    {
        return ParseFrom(Message.SerializePartialAsString());
    }
};
