#include "Logger.h"

#include "SDK/Files/Paths.h"

#include <windows.h>

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace
{
    constexpr const char* LogFileName = "ovr_skins.log";

    HANDLE LogFile = INVALID_HANDLE_VALUE;
    LONG ReportedExceptions = 0;

    bool IsFatal(DWORD Code)
    {
        switch (Code)
        {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_PRIV_INSTRUCTION:
        case EXCEPTION_STACK_OVERFLOW:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_IN_PAGE_ERROR:
            return true;
        default:
            return false;
        }
    }

    void DescribeAddress(std::uintptr_t Address, char* Out, std::size_t Size)
    {
        HMODULE Module = nullptr;
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(Address), &Module))
        {
            std::snprintf(Out, Size, "%#llx", static_cast<unsigned long long>(Address));
            return;
        }

        char Path[MAX_PATH] = {};
        GetModuleFileNameA(Module, Path, MAX_PATH);
        const char* Name = std::strrchr(Path, '\\');
        Name = Name ? Name + 1 : Path;
        std::snprintf(Out, Size, "%s+%#llx", Name, static_cast<unsigned long long>(Address - reinterpret_cast<std::uintptr_t>(Module)));
    }

    LONG CALLBACK ExceptionHandler(EXCEPTION_POINTERS* Info)
    {
        const EXCEPTION_RECORD* Record = Info->ExceptionRecord;
        if (!IsFatal(Record->ExceptionCode) || InterlockedIncrement(&ReportedExceptions) > 10)
            return EXCEPTION_CONTINUE_SEARCH;

        char Location[MAX_PATH + 32];
        DescribeAddress(reinterpret_cast<std::uintptr_t>(Record->ExceptionAddress), Location, sizeof(Location));

        if (Record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && Record->NumberParameters >= 2)
            LogE("Exception %#lx at %s, %s %#llx", Record->ExceptionCode, Location, Record->ExceptionInformation[0] ? "write" : "read", static_cast<unsigned long long>(Record->ExceptionInformation[1]));
        else
            LogE("Exception %#lx at %s", Record->ExceptionCode, Location);
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

bool Logger::Init()
{
    LogFile = CreateFileA(Paths::Resolve(LogFileName).c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (LogFile == INVALID_HANDLE_VALUE)
        return false;

    AddVectoredExceptionHandler(1, ExceptionHandler);
    return true;
}

void Logger::Write(const char* Prefix, const char* Format, ...)
{
    if (LogFile == INVALID_HANDLE_VALUE)
        return;

    char Buffer[4096];
    int Length = std::snprintf(Buffer, sizeof(Buffer), "%s ", Prefix);

    va_list Args;
    va_start(Args, Format);
    int Written = std::vsnprintf(Buffer + Length, sizeof(Buffer) - Length - 1, Format, Args);
    va_end(Args);

    if (Written < 0)
        return;

    Length += Written;
    if (Length > static_cast<int>(sizeof(Buffer)) - 2)
        Length = sizeof(Buffer) - 2;
    Buffer[Length++] = '\n';

    DWORD Unused = 0;
    WriteFile(LogFile, Buffer, static_cast<DWORD>(Length), &Unused, nullptr);
}
