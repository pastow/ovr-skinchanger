#include "SchemaOffset.h"

#include "SDK/Logger/Logger.h"
#include "SDK/SchemaSystem/CSchemaSystem.h"

#include <vector>

namespace
{
    SchemaOffset SchemaInstance;

    bool IsClientModule(const std::string& Module)
    {
        return Module.rfind("client", 0) == 0 && (Module.size() == 6 || Module[6] == '.');
    }
}

SchemaOffset* SchemaOffset::Instance()
{
    return &SchemaInstance;
}

bool SchemaOffset::Init()
{
    CSchemaSystem* System = CSchemaSystem::Get();
    if (!System)
        return false;

    std::vector<CSchemaSystemTypeScope*> Scopes;
    if (CSchemaSystemTypeScope* Global = System->GlobalTypeScope())
        Scopes.push_back(Global);

    std::int32_t ScopeCount = System->TypeScopeCount();
    for (std::int32_t Index = 0; Index < ScopeCount; ++Index)
    {
        if (CSchemaSystemTypeScope* Scope = System->TypeScope(Index))
            Scopes.push_back(Scope);
    }

    std::size_t TotalBindings = 0;
    std::size_t TotalFields = 0;

    for (CSchemaSystemTypeScope* Scope : Scopes)
    {
        for (CSchemaClassBinding* Binding : Scope->Classes())
        {
            ++TotalBindings;

            const char* BindingName = Binding->Name();
            if (!*BindingName)
                continue;

            SchemaClassInfo& Info = SchemaData[BindingName];
            if (!Info.ClassName.empty() && (IsClientModule(Info.ModuleName) || !IsClientModule(Binding->Module())))
                continue;

            Info = SchemaClassInfo {};
            Info.ClassName = BindingName;
            Info.ModuleName = Binding->Module();
            Info.SizeOf = static_cast<std::uint32_t>(Binding->Size());
            Info.Binding = Binding;
            if (CSchemaClassBinding* Base = Binding->BaseClass())
                Info.BaseClassName = Base->Name();

            std::int16_t FieldCount = Binding->FieldCount();
            for (std::int16_t Index = 0; Index < FieldCount; ++Index)
            {
                const SchemaClassFieldData* Data = Binding->Field(Index);
                if (!Data || !*Data->Name())
                    continue;

                SchemaFieldInfo& Field = Info.Fields[Data->Name()];
                Field.FieldName = Data->Name();
                Field.Offset = static_cast<std::uint32_t>(Data->Offset());
                if (CSchemaType* Type = Data->Type())
                    Field.FieldType = Type->Name();
                ++TotalFields;
            }
        }
    }

    Log("Schema scopes %zu, bindings %zu, classes %zu, fields %zu", Scopes.size(), TotalBindings, SchemaData.size(), TotalFields);
    return TotalFields > 0;
}

bool SchemaOffset::HasField(const std::string& ClassName, const std::string& FieldName) const
{
    auto Class = SchemaData.find(ClassName);
    return Class != SchemaData.end() && Class->second.Fields.count(FieldName);
}

std::uint32_t SchemaOffset::GetOffset(const std::string& ClassName, const std::string& FieldName) const
{
    auto Class = SchemaData.find(ClassName);
    if (Class == SchemaData.end())
        return 0;

    auto Field = Class->second.Fields.find(FieldName);
    return Field == Class->second.Fields.end() ? 0 : Field->second.Offset;
}

std::uint32_t SchemaOffset::FindField(const std::string& FieldName, std::string& OutClassName) const
{
    for (const auto& [ClassName, Info] : SchemaData)
    {
        auto Field = Info.Fields.find(FieldName);
        if (Field != Info.Fields.end())
        {
            OutClassName = ClassName;
            return Field->second.Offset;
        }
    }

    OutClassName.clear();
    return 0;
}

CSchemaClassBinding* SchemaOffset::GetClassBinding(const std::string& ClassName) const
{
    auto Class = SchemaData.find(ClassName);
    return Class == SchemaData.end() ? nullptr : Class->second.Binding;
}
