#include "TreasureAddButton.h"

#include "Changer/Gc/ItemGrant.h"
#include "SDK/Econ/CDOTA_UI_EconItem_Animated.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Memory/Memory.h"
#include "SDK/Panorama/Panel.h"
#include "SDK/Panorama/Panorama.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
    constexpr const char* IconsClass = "AttributeIconContainer";
    constexpr const char* TileType = "DOTAEconItemAnimated";
    constexpr const char* InfoButtonId = "TooltipTriggerIcon";
    constexpr const char* ButtonClass = "OvrTreasureAdd";
    constexpr const char* PlusIcon = "s2r://panorama/images/control_icons/24px/plus.vsvg";
    constexpr double Gap = 4.0;

    void Release(std::vector<Panel>& Panels)
    {
        for (Panel& Item : Panels)
            Item.Release();
    }

    void OnAdd(std::intptr_t Argument)
    {
        auto* Native = reinterpret_cast<CUIPanel*>(Argument);
        auto* Tile = CDOTA_UI_EconItem_Animated::FromNative(Native);
        if (!Tile)
        {
            LogE("Treasure add: tile is not CDOTA_UI_EconItem_Animated (%s)",
                 Native ? Memory::RttiName(Native->Client()) : "null");
            return;
        }

        std::uint16_t Def = Tile->Item()->ItemDefinitionIndex();
        CEconItemDefinition* Definition = Tile->ItemDefinition();
        if (Definition && Definition->IsBundle())
        {
            std::vector<std::uint32_t> Defs;
            for (CEconItemDefinition* Item : Definition->BundleItems())
                Defs.push_back(Item->DefIndex());
            if (!Defs.empty())
            {
                ItemGrant::Grant(Defs);
                return;
            }
        }

        ItemGrant::Grant(Def);
    }

    std::string Px(double Value)
    {
        char Buffer[32];
        std::snprintf(Buffer, sizeof(Buffer), "%.1fpx", Value);
        return Buffer;
    }

    void Align(const Panel& Info, const Panel& View, const Panel& Button)
    {
        double Scale = Info.Number("actualuiscale_x");
        double Width = Info.Number("actuallayoutwidth");
        double Height = Info.Number("actuallayoutheight");
        double InfoX = 0.0;
        double InfoY = 0.0;
        double ButtonX = 0.0;
        double ButtonY = 0.0;

        if (Scale <= 0.0 || Width <= 0.0 || Height <= 0.0 || !Info.PositionWithin(View, InfoX, InfoY) || !Button.PositionWithin(View, ButtonX, ButtonY))
            return;

        double X = InfoX + Width + Gap * Scale;
        double Y = InfoY;

        if (std::fabs(ButtonX - X) <= 0.5 && std::fabs(ButtonY - Y) <= 0.5 &&
            std::fabs(Button.Number("actuallayoutwidth") - Width) <= 0.5 && std::fabs(Button.Number("actuallayoutheight") - Height) <= 0.5)
            return;

        Button.SetStyle({
            { "margin-left", Px(X / Scale).c_str() },
            { "margin-top", Px(Y / Scale).c_str() },
            { "width", Px(Width / Scale).c_str() },
            { "height", Px(Height / Scale).c_str() },
        });
    }

    void Create(const Panel& Tile, const Panel& Info, const Panel& View)
    {
        Panel Button = Panel::Create("Button", View, InfoButtonId);
        Button.AddClass("IconButton");
        Button.AddClass(ButtonClass);
        Button.SetEvent("onactivate", OnAdd, reinterpret_cast<std::intptr_t>(Tile.Native()));

        Panel Image = Panel::Create("Image", Button, "OvrTreasureAddIcon");
        Image.SetImage(PlusIcon);

        Align(Info, View, Button);

        Image.Release();
        Button.Release();
    }

    void ProcessTile(const Panel& Tile, const Panel& Info, const Panel& View)
    {
        bool Wanted = !Tile.HasClass("Owned");
        std::vector<Panel> Existing = View.FindChildrenWithClass(ButtonClass);

        if (Existing.empty())
        {
            if (Wanted)
                Create(Tile, Info, View);
            return;
        }

        for (const Panel& Button : Existing)
        {
            Button.SetStyle("visibility", Wanted ? "visible" : "collapse");
            if (Wanted)
                Align(Info, View, Button);
        }
        Release(Existing);
    }

    void ProcessIcons(const Panel& Icons)
    {
        Panel Info = Icons.FindChild(InfoButtonId);
        Panel View = Icons.Parent();
        Panel Container = View.Parent();
        Panel Tile = Container.Parent();

        if (Info.Handle() && Tile.Type() == TileType)
            ProcessTile(Tile, Info, View);

        Tile.Release();
        Container.Release();
        View.Release();
        Info.Release();
    }
}

void TreasureAddButton::Update()
{
    Panorama::Scope Scope;
    if (!Scope.IsValid())
        return;

    Panel Root = Panel::Root();
    std::vector<Panel> IconContainers = Root.FindChildrenWithClass(IconsClass);
    for (const Panel& Icons : IconContainers)
        ProcessIcons(Icons);

    Release(IconContainers);
    Root.Release();
}
