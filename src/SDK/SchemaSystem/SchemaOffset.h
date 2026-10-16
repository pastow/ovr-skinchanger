#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

class CSchemaClassBinding;

struct SchemaFieldInfo
{
    std::string FieldName;
    std::string FieldType;
    std::uint32_t Offset = 0;
};

struct SchemaClassInfo
{
    std::string ClassName;
    std::string ModuleName;
    std::string BaseClassName;
    std::uint32_t SizeOf = 0;
    CSchemaClassBinding* Binding = nullptr;
    std::unordered_map<std::string, SchemaFieldInfo> Fields;
};

class SchemaOffset
{
public:
    static SchemaOffset* Instance();

    bool Init();
    bool HasField(const std::string& ClassName, const std::string& FieldName) const;
    std::uint32_t GetOffset(const std::string& ClassName, const std::string& FieldName) const;
    std::uint32_t FindField(const std::string& FieldName, std::string& OutClassName) const;
    CSchemaClassBinding* GetClassBinding(const std::string& ClassName) const;

private:
    std::unordered_map<std::string, SchemaClassInfo> SchemaData;
};
