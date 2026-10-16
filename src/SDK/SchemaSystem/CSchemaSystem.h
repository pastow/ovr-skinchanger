#pragma once

#include <cstdint>
#include <vector>

class CSchemaType
{
public:
    const char* Name() const;
};

class SchemaClassFieldData
{
public:
    const char* Name() const;
    CSchemaType* Type() const;
    std::int32_t Offset() const;
};

class CSchemaClassBinding
{
public:
    const char* Name() const;
    const char* Module() const;
    std::int32_t Size() const;
    std::int16_t FieldCount() const;
    const SchemaClassFieldData* Field(std::int32_t Index) const;
    CSchemaClassBinding* BaseClass() const;
};

class CSchemaSystemTypeScope
{
public:
    const char* Name() const;
    std::vector<CSchemaClassBinding*> Classes() const;
};

class CSchemaSystem
{
public:
    static bool Init();
    static CSchemaSystem* Get();

    CSchemaSystemTypeScope* GlobalTypeScope();
    std::int32_t TypeScopeCount() const;
    CSchemaSystemTypeScope* TypeScope(std::int32_t Index) const;
};
