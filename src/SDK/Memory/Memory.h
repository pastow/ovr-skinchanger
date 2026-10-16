#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace Memory
{
    struct ModuleInfo
    {
        std::uintptr_t Base = 0;
        std::size_t Size = 0;
        std::vector<std::pair<std::uintptr_t, std::size_t>> ExecutableSections;
    };

    template <typename T>
    T Read(std::uintptr_t Address)
    {
        return *reinterpret_cast<T*>(Address);
    }

    bool GetModuleInfo(const char* ModuleName, ModuleInfo& Out);

    std::uintptr_t FindPattern(std::uintptr_t Base, std::size_t Size, const char* Pattern);
    std::uintptr_t FindPattern(const char* ModuleName, const char* Pattern);
    std::uintptr_t ResolveRelative(std::uintptr_t Address, std::size_t DisplacementOffset, std::size_t InstructionSize);

    const char* RttiName(const void* Object);

    bool SetProtection(void* Address, std::size_t Size, unsigned long Protection, unsigned long& OldProtection);
}

class VmtHook
{
public:
    bool Install(void* Instance, std::size_t Slot, void* NewFn);
    bool Uninstall();

    void* Original() const { return OriginalFn; }
    bool IsInstalled() const { return Installed; }

private:
    void** Vtable = nullptr;
    std::size_t SlotIndex = 0;
    void* OriginalFn = nullptr;
    bool Installed = false;
};
