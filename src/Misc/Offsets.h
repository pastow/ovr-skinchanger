#pragma once

#include <cstddef>
#include <cstdint>

namespace Offsets::UIEngine
{
    inline constexpr std::ptrdiff_t DocScopeCount = 0x390;
    inline constexpr std::ptrdiff_t DocScopes = 0x398;
    inline constexpr std::ptrdiff_t MainContext = 0x5C0;
    inline constexpr std::ptrdiff_t Isolate = 0x5F0;
    inline constexpr std::ptrdiff_t PanelContextsCount = 0x650;
    inline constexpr std::ptrdiff_t PanelContextsCapacity = 0x654;
    inline constexpr std::ptrdiff_t PanelContexts = 0x658;
}

namespace Offsets::PanelContextEntry
{
    inline constexpr std::size_t Size = 0x20;
    inline constexpr std::ptrdiff_t Index = 0x0;
    inline constexpr std::ptrdiff_t Context = 0x18;
}

namespace Offsets::PanelObject
{
    inline constexpr int InternalFieldCount = 2;
    inline constexpr int UIPanelField = 1;
}

namespace Offsets::CUIPanel
{
    inline constexpr std::ptrdiff_t ClientPtr = 0x40;
}

namespace Offsets::CDOTA_DB_Page_OwnedItemDetails
{
    inline constexpr std::ptrdiff_t Item = 0x358;
}

namespace Offsets::CGCClientSystem
{
    inline constexpr std::ptrdiff_t GCClient = 0xA0;
}

namespace Offsets::CGCClient
{
    inline constexpr std::ptrdiff_t SteamGameCoordinator = 0x18;
}

namespace Offsets::CDOTA_UI_EconItem_Animated
{
    inline constexpr std::ptrdiff_t Item = 0x68;
    inline constexpr std::ptrdiff_t ItemDefinition = 0x1B8;
}

namespace Offsets::CEconItemDefinition
{
    inline constexpr std::ptrdiff_t DefIndex = 0x28;
    inline constexpr std::ptrdiff_t BundleInfo = 0xE0;
    inline constexpr std::ptrdiff_t LoadoutSlot = 0x188;
    inline constexpr std::ptrdiff_t HeroId = 0x18C;
    inline constexpr std::size_t GetAssetModifiersIndex = 20;
    inline constexpr std::ptrdiff_t Models = 0xA0;
    inline constexpr std::ptrdiff_t Visuals = 0xD0;
}

namespace Offsets::ItemVisuals
{
    inline constexpr std::ptrdiff_t Skin = 0x38;
    inline constexpr std::ptrdiff_t Styles = 0x48;
}

namespace Offsets::ItemStyle
{
    inline constexpr std::ptrdiff_t Skin = 0xC;
    inline constexpr std::ptrdiff_t Model = 0x20;
}

namespace Offsets::CModel
{
    inline constexpr std::ptrdiff_t MaterialGroupCount = 0x298;
    inline constexpr std::ptrdiff_t MaterialGroups = 0x2A0;
}

namespace Offsets::AssetModifierEntry
{
    inline constexpr std::size_t Size = 0x30;
    inline constexpr int Capacity = 512;
}

namespace Offsets::BundleInfo
{
    inline constexpr std::ptrdiff_t Count = 0x8;
    inline constexpr std::ptrdiff_t Items = 0x10;
}

namespace Offsets::C_EconItemView
{
    inline constexpr std::ptrdiff_t ItemDefinitionIndex = 0x8;
    inline constexpr std::ptrdiff_t ItemId = 0x18;
    inline constexpr std::ptrdiff_t AccountId = 0x20;
    inline constexpr std::ptrdiff_t Style = 0x31;
    inline constexpr std::size_t Size = 0xD8;
}

namespace Offsets::IEconItemInterface
{
    inline constexpr std::size_t GetItemIdIndex = 14;
}

namespace Offsets::CResourceSystem
{
    inline constexpr std::size_t CreateManifestIndex = 22;
    inline constexpr std::size_t ReleaseManifestIndex = 25;
    inline constexpr std::size_t FindBindingIndex = 29;
    inline constexpr std::size_t StatusByIdIndex = 50;
}

namespace Offsets::C_BaseEntity
{
    inline constexpr std::ptrdiff_t ParticleProperty = 0x568;
}

namespace Offsets::C_DOTA_BaseNPC
{
    inline constexpr std::size_t SelectWeightedSequenceIndex = 269;
}

namespace Offsets::ResourceBinding
{
    inline constexpr std::ptrdiff_t Name = 0x8;
    inline constexpr std::ptrdiff_t Id = 0x10;
}

namespace Offsets::ProtobufMessage
{
    inline constexpr std::size_t ClearIndex = 3;
    inline constexpr std::size_t ByteSizeLongIndex = 7;
    inline constexpr std::size_t InternalParseIndex = 9;
    inline constexpr std::size_t InternalSerializeIndex = 11;
}

namespace Offsets::CNetMessage
{
    inline constexpr std::size_t ProtoIndex = 1;
    inline constexpr std::size_t IdIndex = 5;
}

namespace Offsets::CNetworkClientService
{
    inline constexpr std::size_t GameClientIndex = 23;
}

namespace Offsets::CNetworkServerService
{
    inline constexpr std::size_t SerializersIndex = 42;
}

namespace Offsets::CNetworkGameClient
{
    inline constexpr std::ptrdiff_t ClassCount = 0x170;
    inline constexpr std::ptrdiff_t Classes = 0x178;
    inline constexpr std::size_t ClassEntrySize = 0x10;
    inline constexpr std::ptrdiff_t SerializerCache = 0x1C0;
    inline constexpr std::ptrdiff_t SerializerProto = 0x30;
}

namespace Offsets::CDOTAInventoryManager
{
    inline constexpr std::size_t LocalInventoryIndex = 67;
}

namespace Offsets::CDOTAPlayerInventory
{
    inline constexpr std::ptrdiff_t OwnerId = 0x8;
    inline constexpr std::ptrdiff_t OwnerType = 0x10;
}

namespace Offsets::KeyValues3
{
    inline constexpr std::ptrdiff_t Data = 0x8;
}

namespace Offsets::EntitySpawnInfo
{
    inline constexpr std::ptrdiff_t KeyValues = 0x0;
}

namespace Offsets::CGameEntitySystem
{
    inline constexpr std::ptrdiff_t IdentityChunks = 0x10;
    inline constexpr int ChunkSize = 512;
    inline constexpr std::size_t IdentityStride = 0x70;
}

namespace Offsets::CEntityIdentity
{
    inline constexpr std::ptrdiff_t Entity = 0x0;
    inline constexpr std::ptrdiff_t Handle = 0x10;
    inline constexpr std::ptrdiff_t Name = 0x18;
    inline constexpr std::ptrdiff_t DesignerName = 0x20;
}

namespace Offsets::CUtlVector
{
    inline constexpr std::ptrdiff_t Size = 0x0;
    inline constexpr std::ptrdiff_t Data = 0x8;
}

namespace Offsets::CSchemaSystem
{
    inline constexpr std::size_t GlobalTypeScopeIndex = 11;
    inline constexpr std::ptrdiff_t TypeScopeCount = 0x190;
    inline constexpr std::ptrdiff_t TypeScopes = 0x198;
}

namespace Offsets::CSchemaSystemTypeScope
{
    inline constexpr std::ptrdiff_t Name = 0x8;
    inline constexpr std::ptrdiff_t ClassContainer = 0x5C0;
}

namespace Offsets::CSchemaList
{
    inline constexpr std::size_t BlockCount = 256;
    inline constexpr std::size_t BlockContainerSize = 0x18;
    inline constexpr std::ptrdiff_t FirstBlock = 0x10;
    inline constexpr std::ptrdiff_t NextBlock = 0x8;
    inline constexpr std::ptrdiff_t Binding = 0x10;
}

namespace Offsets::CSchemaClassBinding
{
    inline constexpr std::ptrdiff_t Name = 0x8;
    inline constexpr std::ptrdiff_t Module = 0x10;
    inline constexpr std::ptrdiff_t Size = 0x20;
    inline constexpr std::ptrdiff_t FieldCount = 0x24;
    inline constexpr std::ptrdiff_t Fields = 0x30;
    inline constexpr std::ptrdiff_t BaseClass = 0x38;
    inline constexpr std::ptrdiff_t BaseClassBinding = 0x8;
}

namespace Offsets::SchemaClassFieldData
{
    inline constexpr std::size_t Size = 0x20;
    inline constexpr std::ptrdiff_t Name = 0x0;
    inline constexpr std::ptrdiff_t Type = 0x8;
    inline constexpr std::ptrdiff_t Offset = 0x10;
}

namespace Offsets::CSchemaType
{
    inline constexpr std::ptrdiff_t Name = 0x8;
}

namespace Offsets::DocScope
{
    inline constexpr std::size_t Size = 0x28;
    inline constexpr std::ptrdiff_t Name = 0x0;
    inline constexpr std::ptrdiff_t Description = 0x8;
    inline constexpr std::ptrdiff_t FunctionCount = 0x10;
    inline constexpr std::ptrdiff_t Functions = 0x18;
}

namespace Offsets::DocFunction
{
    inline constexpr std::size_t Size = 0x20;
    inline constexpr std::ptrdiff_t Name = 0x0;
    inline constexpr std::ptrdiff_t Description = 0x8;
    inline constexpr std::ptrdiff_t Kind = 0x10;
    inline constexpr std::ptrdiff_t ReturnType = 0x14;
    inline constexpr std::ptrdiff_t ArgCount = 0x15;
    inline constexpr std::ptrdiff_t ArgTypes = 0x16;
    inline constexpr std::size_t MaxArgs = 10;
}

namespace Offsets::V8
{
    inline constexpr std::ptrdiff_t MapInstanceType = 0xC;
    inline constexpr std::ptrdiff_t JSObjectElements = 0x10;
    inline constexpr std::ptrdiff_t JSFunctionShared = 0x20;
    inline constexpr std::ptrdiff_t SharedFunctionData = 0x8;
    inline constexpr std::ptrdiff_t FunctionTemplateCallCode = 0x30;
    inline constexpr std::ptrdiff_t CallHandlerData = 0x8;
    inline constexpr std::ptrdiff_t CallHandlerCallback = 0x18;
    inline constexpr std::ptrdiff_t ExternalValue = 0x18;
    inline constexpr std::ptrdiff_t FixedArrayLength = 0x8;
    inline constexpr std::ptrdiff_t FixedArrayData = 0x10;
}

namespace Offsets::FunctionCallbackInfo
{
    inline constexpr std::ptrdiff_t ImplicitArgs = 0x0;
    inline constexpr std::ptrdiff_t Values = 0x8;
    inline constexpr std::ptrdiff_t Length = 0x10;
    inline constexpr std::size_t IsolateIndex = 1;
    inline constexpr std::size_t DataIndex = 4;
}

namespace Offsets::V8Type
{
    inline constexpr std::uint16_t FunctionTemplateInfo = 139;
    inline constexpr std::uint16_t CallHandlerInfo = 236;
    inline constexpr std::uint16_t SharedFunctionInfo = 255;
    inline constexpr std::uint16_t JSObjectFirst = 0x108;
    inline constexpr std::uint16_t JSFunctionFirst = 2066;
    inline constexpr std::uint16_t JSFunctionLast = 2081;
    inline constexpr std::uint16_t JSExternalObject = 2124;
}
