#pragma once

#include <cstdint>
#include <string>

namespace V8
{
    using Address = std::uintptr_t;
    using Local = Address*;
    using Callback = void (*)(const void* Info);

    struct HandleScope
    {
        alignas(8) unsigned char Storage[24];
    };

    struct TryCatch
    {
        alignas(8) unsigned char Storage[48];
    };

    struct NativeInfo
    {
        bool Api = false;
        std::uintptr_t Callback = 0;
        std::uintptr_t Method = 0;
        std::uintptr_t Adjust = 0;
    };

    bool Init();

    void IsolateEnter(void* Isolate);
    void IsolateExit(void* Isolate);

    void OpenHandleScope(HandleScope& Scope, void* Isolate);
    void CloseHandleScope(HandleScope& Scope);
    void OpenTryCatch(TryCatch& Catch, void* Isolate);
    void CloseTryCatch(TryCatch& Catch);
    bool HasCaught(const TryCatch& Catch);
    Local Exception(const TryCatch& Catch);
    void ResetTryCatch(TryCatch& Catch);
    Local CreateHandle(void* Isolate, Address Value);

    void ContextEnter(Local Context);
    void ContextExit(Local Context);
    Local ContextGlobal(Local Context);

    Local NewString(void* Isolate, const char* Text);
    Local GetPropertyNames(Local Object, Local Context);
    std::uint32_t ArrayLength(Local Array);
    Local GetIndex(Local Object, Local Context, std::uint32_t Index);
    Local Get(Local Object, Local Context, Local Key);
    Local Get(void* Isolate, Local Object, Local Context, const char* Key);
    Local Call(Local Function, Local Context, Local Receiver, int Argc, Local* Argv);

    bool IsFunction(Local Value);
    bool IsObject(Local Value);
    std::string ToString(void* Isolate, Local Value);
    std::string TypeOf(void* Isolate, Local Value);

    Local NewNumber(void* Isolate, double Value);
    Local NewFunction(void* Isolate, Local Context, Callback Function, void* Data);
    bool Set(Local Object, Local Context, Local Key, Local Value);
    Address* Globalize(void* Isolate, Local Value);
    void DisposeGlobal(Address* Handle);
    bool BooleanValue(void* Isolate, Local Value);
    double NumberValue(Local Value, Local Context);
    void* CallbackData(const void* Info);
    int InternalFieldCount(Local Object);
    Local GetInternalField(Local Object, int Index);
    void* ExternalValue(Local Value);

    bool GetNativeInfo(Local Function, NativeInfo& Out);
}
