#include "CProtobufMessage.h"

#include "Misc/Offsets.h"
#include "SDK/Memory/Memory.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/parse_context.h>

#include <cstdint>

namespace
{
    template <typename Fn>
    Fn Virtual(const void* Object, std::size_t Index)
    {
        return reinterpret_cast<Fn>(Memory::Read<void**>(reinterpret_cast<std::uintptr_t>(Object))[Index]);
    }
}

bool CProtobufMessage::SerializeTo(std::string& Out) const
{
    using namespace google::protobuf;

    std::size_t Size = Virtual<std::size_t (*)(const CProtobufMessage*)>(this, Offsets::ProtobufMessage::ByteSizeLongIndex)(this);
    if (Size > static_cast<std::size_t>(INT32_MAX))
        return false;
    Out.resize(Size);
    auto* Target = reinterpret_cast<std::uint8_t*>(Out.data());
    io::EpsCopyOutputStream Stream(Target, static_cast<int>(Size), io::CodedOutputStream::IsDefaultSerializationDeterministic());
    std::uint8_t* End = Virtual<std::uint8_t* (*)(const CProtobufMessage*, std::uint8_t*, io::EpsCopyOutputStream*)>(
        this, Offsets::ProtobufMessage::InternalSerializeIndex)(this, Target, &Stream);
    return End == Target + Size;
}

bool CProtobufMessage::ParseFrom(const std::string& Data)
{
    using namespace google::protobuf;

    Virtual<void (*)(CProtobufMessage*)>(this, Offsets::ProtobufMessage::ClearIndex)(this);
    const char* Ptr = nullptr;
    internal::ParseContext Context(io::CodedInputStream::GetDefaultRecursionLimit(), false, &Ptr, StringPiece(Data));
    Ptr = Virtual<const char* (*)(CProtobufMessage*, const char*, internal::ParseContext*)>(this, Offsets::ProtobufMessage::InternalParseIndex)(
        this, Ptr, &Context);
    return Ptr && Context.EndedAtLimit();
}
