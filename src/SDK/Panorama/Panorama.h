#pragma once

#include "SDK/V8/V8.h"

#include <cstdint>

namespace Panorama
{
    using Handler = void (*)(std::intptr_t Argument);

    class Scope
    {
    public:
        Scope();
        ~Scope();

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

        bool IsValid() const;

    private:
        void* Isolate = nullptr;
        V8::Local Context = nullptr;
        V8::Local PreviousContext = nullptr;
        void* PreviousIsolate = nullptr;
        V8::HandleScope Handles {};
        V8::TryCatch Catch {};
    };

    void* Isolate();
    V8::Local Context();
    V8::Local Global();
    V8::Local Dollar();
    V8::Local Root();

    V8::Local String(const char* Text);
    V8::Local Number(double Value);
    V8::Local Function(Handler Callback, std::intptr_t Argument);
    V8::Local Invoke(V8::Local Object, const char* Method, int Argc = 0, V8::Local* Argv = nullptr);
}
