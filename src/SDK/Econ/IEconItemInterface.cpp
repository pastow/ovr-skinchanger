#include "IEconItemInterface.h"

#include "Misc/Offsets.h"
#include "SDK/Memory/Memory.h"

std::uint64_t IEconItemInterface::ItemId() const
{
    using GetItemIdFn = std::uint64_t* (*)(const IEconItemInterface*, std::uint64_t*);
    auto** Vtable = Memory::Read<void**>(reinterpret_cast<std::uintptr_t>(this));
    std::uint64_t Out = 0;
    std::uint64_t* Result = reinterpret_cast<GetItemIdFn>(Vtable[Offsets::IEconItemInterface::GetItemIdIndex])(this, &Out);
    return Result ? *Result : 0;
}
