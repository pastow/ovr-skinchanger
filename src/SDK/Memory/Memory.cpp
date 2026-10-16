#include "Memory.h"

#include "SDK/Logger/Logger.h"

#include <windows.h>

#include <cctype>
#include <cstdlib>

namespace
{
    bool ParsePattern(const char* Pattern, std::vector<int>& OutBytes)
    {
        OutBytes.clear();

        const char* Cursor = Pattern;
        while (*Cursor)
        {
            while (*Cursor == ' ' || *Cursor == '\t')
                ++Cursor;
            if (!*Cursor)
                break;

            if (*Cursor == '?')
            {
                OutBytes.push_back(-1);
                ++Cursor;
                if (*Cursor == '?')
                    ++Cursor;
                continue;
            }

            char Buffer[3] = { 0, 0, 0 };
            int Length = 0;
            while (Length < 2 && std::isxdigit(static_cast<unsigned char>(*Cursor)))
                Buffer[Length++] = *Cursor++;

            if (Length == 0)
                return false;

            OutBytes.push_back(static_cast<int>(std::strtol(Buffer, nullptr, 16)) & 0xFF);
        }

        return !OutBytes.empty();
    }
}

bool Memory::GetModuleInfo(const char* ModuleName, ModuleInfo& Out)
{
    HMODULE Module = GetModuleHandleA(ModuleName);
    if (!Module)
        return false;

    auto Base = reinterpret_cast<std::uintptr_t>(Module);
    auto* Dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(Base);
    if (Dos->e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    auto* Nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(Base + Dos->e_lfanew);
    if (Nt->Signature != IMAGE_NT_SIGNATURE)
        return false;

    Out.Base = Base;
    Out.Size = Nt->OptionalHeader.SizeOfImage;
    Out.ExecutableSections.clear();

    const IMAGE_SECTION_HEADER* Section = IMAGE_FIRST_SECTION(Nt);
    for (WORD Index = 0; Index < Nt->FileHeader.NumberOfSections; ++Index, ++Section)
    {
        if (Section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
            Out.ExecutableSections.emplace_back(Base + Section->VirtualAddress, Section->Misc.VirtualSize);
    }
    return true;
}

std::uintptr_t Memory::FindPattern(std::uintptr_t Base, std::size_t Size, const char* Pattern)
{
    if (!Base || !Size || !Pattern)
        return 0;

    std::vector<int> Bytes;
    if (!ParsePattern(Pattern, Bytes))
    {
        LogE("Failed to parse pattern: %s", Pattern);
        return 0;
    }

    if (Size < Bytes.size())
        return 0;

    auto* Data = reinterpret_cast<const std::uint8_t*>(Base);
    std::size_t PatternLength = Bytes.size();
    std::size_t Last = Size - PatternLength;

    for (std::size_t Offset = 0; Offset <= Last; ++Offset)
    {
        bool Match = true;
        for (std::size_t Index = 0; Index < PatternLength; ++Index)
        {
            int Byte = Bytes[Index];
            if (Byte != -1 && Data[Offset + Index] != Byte)
            {
                Match = false;
                break;
            }
        }
        if (Match)
            return Base + Offset;
    }

    return 0;
}

std::uintptr_t Memory::FindPattern(const char* ModuleName, const char* Pattern)
{
    ModuleInfo Info;
    if (!GetModuleInfo(ModuleName, Info))
    {
        LogE("Module not loaded: %s", ModuleName);
        return 0;
    }

    for (const auto& [Start, Size] : Info.ExecutableSections)
    {
        std::uintptr_t Result = FindPattern(Start, Size, Pattern);
        if (Result)
            return Result;
    }

    LogE("Pattern not found in %s: %s", ModuleName, Pattern);
    return 0;
}

std::uintptr_t Memory::ResolveRelative(std::uintptr_t Address, std::size_t DisplacementOffset, std::size_t InstructionSize)
{
    if (!Address)
        return 0;

    std::int32_t Displacement = Read<std::int32_t>(Address + DisplacementOffset);
    return Address + InstructionSize + Displacement;
}

const char* Memory::RttiName(const void* Object)
{
    if (!Object)
        return nullptr;

    auto Vtable = Read<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(Object));
    if (!Vtable)
        return nullptr;

    auto Locator = Read<std::uintptr_t>(Vtable - sizeof(void*));
    if (!Locator || Read<std::uint32_t>(Locator) != 1)
        return nullptr;

    std::uintptr_t Base = Locator - Read<std::uint32_t>(Locator + 0x14);
    std::uintptr_t Descriptor = Base + Read<std::uint32_t>(Locator + 0xC);
    return reinterpret_cast<const char*>(Descriptor + 0x10);
}

bool Memory::SetProtection(void* Address, std::size_t Size, unsigned long Protection, unsigned long& OldProtection)
{
    DWORD Previous = 0;
    if (!VirtualProtect(Address, Size, Protection, &Previous))
        return false;
    OldProtection = Previous;
    return true;
}

bool VmtHook::Install(void* Instance, std::size_t Slot, void* NewFn)
{
    if (Installed || !Instance || !NewFn)
        return false;

    Vtable = *reinterpret_cast<void***>(Instance);
    if (!Vtable)
    {
        LogE("Null vtable");
        return false;
    }

    SlotIndex = Slot;
    void** SlotAddress = &Vtable[SlotIndex];

    unsigned long OldProtection = 0;
    if (!Memory::SetProtection(SlotAddress, sizeof(void*), PAGE_READWRITE, OldProtection))
    {
        LogE("VirtualProtect failed: %lu", GetLastError());
        Vtable = nullptr;
        return false;
    }

    OriginalFn = *SlotAddress;
    *SlotAddress = NewFn;

    unsigned long Unused = 0;
    if (!Memory::SetProtection(SlotAddress, sizeof(void*), OldProtection, Unused))
        LogW("VirtualProtect restore failed: %lu", GetLastError());

    Installed = true;
    return true;
}

bool VmtHook::Uninstall()
{
    if (!Installed)
        return false;

    void** SlotAddress = &Vtable[SlotIndex];

    unsigned long OldProtection = 0;
    if (!Memory::SetProtection(SlotAddress, sizeof(void*), PAGE_READWRITE, OldProtection))
    {
        LogE("VirtualProtect failed: %lu", GetLastError());
        return false;
    }

    *SlotAddress = OriginalFn;

    unsigned long Unused = 0;
    if (!Memory::SetProtection(SlotAddress, sizeof(void*), OldProtection, Unused))
        LogW("VirtualProtect restore failed: %lu", GetLastError());

    Installed = false;
    OriginalFn = nullptr;
    Vtable = nullptr;
    return true;
}
