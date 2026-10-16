#include "Paths.h"

#include <windows.h>

std::string Paths::Resolve(const char* FileName)
{
    HMODULE Module = nullptr;
    char Path[MAX_PATH] = {};
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(&Paths::Resolve), &Module) ||
        !GetModuleFileNameA(Module, Path, MAX_PATH))
        return FileName;

    std::string Directory(Path);
    std::size_t Slash = Directory.find_last_of("\\/");
    return Slash == std::string::npos ? std::string(FileName) : Directory.substr(0, Slash + 1) + FileName;
}
