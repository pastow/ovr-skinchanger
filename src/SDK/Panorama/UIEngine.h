#pragma once

#include "SDK/V8/V8.h"

#include <cstddef>
#include <cstdint>

class DocFunction
{
public:
    const char* Name() const;
    const char* Description() const;
    std::int32_t Kind() const;
    std::uint8_t ReturnType() const;
    std::uint8_t ArgCount() const;
    std::uint8_t ArgType(std::size_t Index) const;
};

class DocScope
{
public:
    const char* Name() const;
    const char* Description() const;
    std::int32_t FunctionCount() const;
    const DocFunction* Function(std::int32_t Index) const;
};

class UIEngine
{
public:
    static bool Init();
    static UIEngine* Get();

    void* Isolate() const;
    V8::Address* MainContext() const;
    std::int32_t PanelContextCount() const;
    V8::Address* PanelContext(std::int32_t Index) const;
    std::int32_t DocScopeCount() const;
    const DocScope* Scope(std::int32_t Index) const;
};
