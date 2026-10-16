#include "UnusualSettings.h"

#include "Changer/Unusual/UnusualEffects.h"
#include "Changer/Unusual/UnusualStore.h"
#include "SDK/Dashboard/CDOTA_DB_Page_OwnedItemDetails.h"
#include "SDK/Logger/Logger.h"
#include "SDK/Panorama/Panel.h"
#include "SDK/Panorama/Panorama.h"

#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <string>

namespace
{
    constexpr const char* SectionId = "OvrUnusualSettings";
    constexpr const char* DropDownId = "OvrUnusualSettingsEffect";
    constexpr const char* OptionPrefix = "OvrUnusualEffect";

    std::uint64_t SyncedItemId = 0;
    bool Syncing = false;

    void AddClasses(const Panel& Target, std::initializer_list<const char*> Classes)
    {
        for (const char* Class : Classes)
            Target.AddClass(Class);
    }

    std::string OptionId(int EffectId)
    {
        return OptionPrefix + std::to_string(EffectId);
    }

    std::uint64_t CurrentItemId(const Panel& Body)
    {
        Panel Page = Body.FindAncestor("Page");
        CDOTA_DB_Page_OwnedItemDetails* Native = CDOTA_DB_Page_OwnedItemDetails::FromPanel(Page);
        Page.Release();
        return Native ? Native->Item()->ItemId() : 0;
    }

    void OnSelect(std::intptr_t)
    {
        if (Syncing)
            return;

        Panel Root = Panel::Root();
        Panel Picker = Root.FindChildTraverse(DropDownId);
        Panel Option = Picker.Selected();

        std::string Id = Option.Id();
        std::uint64_t ItemId = CurrentItemId(Picker);
        if (ItemId && Id.rfind(OptionPrefix, 0) == 0)
        {
            int EffectId = std::atoi(Id.c_str() + std::char_traits<char>::length(OptionPrefix));
            if (UnusualEffects::Find(EffectId) && EffectId != UnusualStore::Get(ItemId))
                UnusualStore::Set(ItemId, EffectId);
        }

        Option.Release();
        Picker.Release();
        Root.Release();
    }

    void BuildPicker(const Panel& Header)
    {
        Panel Picker = Panel::Create("DropDown", Header, DropDownId);
        Picker.SetStyle({
            { "horizontal-align", "right" },
            { "vertical-align", "center" },
            { "margin", "8px" },
        });

        for (const UnusualEffects::Effect& Entry : UnusualEffects::All())
        {
            Panel Option = Panel::Create("Label", Picker, OptionId(Entry.Id).c_str());
            Option.SetText(Entry.Name);
            Picker.AddOption(Option);
            Option.Release();
        }

        Picker.SetSelected(OptionId(0).c_str());
        Picker.SetEvent("oninputsubmit", OnSelect, 0);
        Picker.Release();
    }

    void Build(const Panel& Body)
    {
        Panel Section = Panel::Create("Panel", Body, SectionId);
        AddClasses(Section, { "EditSection", "TopBottomFlow" });

        Panel Header = Panel::Create("Panel", Section, "OvrUnusualSettingsHeader");
        AddClasses(Header, { "EditSectionHeader", "LeftRightFlow" });
        Header.SetStyle("height", "fit-children");

        Panel Title = Panel::Create("Label", Header, "OvrUnusualSettingsTitle");
        AddClasses(Title, { "HeaderText", "LabelLarge", "LightestGreyText" });
        Title.SetText("Unusual settings");

        BuildPicker(Header);
        SyncedItemId = 0;


        Title.Release();
        Header.Release();
        Section.Release();
    }

    void Sync(const Panel& Body)
    {
        std::uint64_t ItemId = CurrentItemId(Body);
        if (!ItemId || ItemId == SyncedItemId)
            return;

        Panel Picker = Body.FindChildTraverse(DropDownId);
        if (!Picker.Handle())
            return;

        int EffectId = UnusualStore::Get(ItemId);
        Syncing = true;
        Picker.SetSelected(OptionId(EffectId).c_str());
        Syncing = false;
        Picker.Release();

        SyncedItemId = ItemId;
    }
}

void UnusualSettings::Update()
{
    Panorama::Scope Scope;
    if (!Scope.IsValid())
        return;

    Panel Root = Panel::Root();
    Panel Body = Root.FindChildTraverse("ItemEditBody");

    if (Body.Handle())
    {
        Panel Existing = Body.FindChild(SectionId);
        if (!Existing.Handle())
            Build(Body);
        Existing.Release();

        Sync(Body);
    }

    Body.Release();
    Root.Release();
}
