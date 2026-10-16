#include "Panel.h"

#include "Misc/Offsets.h"

Panel::Panel(V8::Local Object)
{
    if (Object && V8::IsObject(Object))
        Global = V8::Globalize(Panorama::Isolate(), Object);
}

Panel Panel::Root()
{
    return Panel(Panorama::Root());
}

Panel Panel::Create(const char* Type, const Panel& Parent, const char* Id)
{
    V8::Local Args[] = { Panorama::String(Type), Parent.Handle(), Panorama::String(Id) };
    if (!Args[1])
        return Panel();
    return Panel(Panorama::Invoke(Panorama::Dollar(), "CreatePanel", 3, Args));
}

Panel Panel::Create(const char* Type, const Panel& Parent, const char* Id, Style Styles)
{
    Panel Result = Create(Type, Parent, Id);
    Result.SetStyle(Styles);
    return Result;
}

Panel Panel::FindChild(const char* Id) const
{
    V8::Local Args[] = { Panorama::String(Id) };
    return Panel(Panorama::Invoke(Handle(), "FindChild", 1, Args));
}

Panel Panel::FindChildTraverse(const char* Id) const
{
    V8::Local Args[] = { Panorama::String(Id) };
    return Panel(Panorama::Invoke(Handle(), "FindChildTraverse", 1, Args));
}

std::vector<Panel> Panel::FromArray(V8::Local Array)
{
    std::vector<Panel> Result;
    if (!Array || !V8::IsObject(Array))
        return Result;

    std::uint32_t Count = V8::ArrayLength(Array);
    Result.reserve(Count);
    for (std::uint32_t Index = 0; Index < Count; ++Index)
        Result.push_back(Panel(V8::GetIndex(Array, Panorama::Context(), Index)));
    return Result;
}

std::vector<Panel> Panel::FindChildrenWithClass(const char* Class) const
{
    V8::Local Args[] = { Panorama::String(Class) };
    return FromArray(Panorama::Invoke(Handle(), "FindChildrenWithClassTraverse", 1, Args));
}

std::vector<Panel> Panel::Children() const
{
    return FromArray(Panorama::Invoke(Handle(), "Children"));
}

Panel Panel::Parent() const
{
    return Panel(Panorama::Invoke(Handle(), "GetParent"));
}

Panel Panel::FindAncestor(const char* Id) const
{
    V8::Local Args[] = { Panorama::String(Id) };
    return Panel(Panorama::Invoke(Handle(), "FindAncestor", 1, Args));
}

std::string Panel::Type() const
{
    V8::Local Object = Handle();
    if (!Object)
        return std::string();

    V8::Local Value = V8::Get(Panorama::Isolate(), Object, Panorama::Context(), "paneltype");
    return Value ? V8::ToString(Panorama::Isolate(), Value) : std::string();
}

double Panel::Number(const char* Property) const
{
    V8::Local Object = Handle();
    if (!Object)
        return 0.0;

    V8::Local Value = V8::Get(Panorama::Isolate(), Object, Panorama::Context(), Property);
    return Value ? V8::NumberValue(Value, Panorama::Context()) : 0.0;
}

bool Panel::PositionWithin(const Panel& Ancestor, double& X, double& Y) const
{
    V8::Local Args[] = { Ancestor.Handle() };
    if (!Args[0])
        return false;

    V8::Local Position = Panorama::Invoke(Handle(), "GetPositionWithinAncestor", 1, Args);
    if (!Position || !V8::IsObject(Position))
        return false;

    V8::Local XValue = V8::Get(Panorama::Isolate(), Position, Panorama::Context(), "x");
    V8::Local YValue = V8::Get(Panorama::Isolate(), Position, Panorama::Context(), "y");
    if (!XValue || !YValue)
        return false;

    X = V8::NumberValue(XValue, Panorama::Context());
    Y = V8::NumberValue(YValue, Panorama::Context());
    return true;
}

bool Panel::HasClass(const char* Class) const
{
    V8::Local Args[] = { Panorama::String(Class) };
    V8::Local Result = Panorama::Invoke(Handle(), "BHasClass", 1, Args);
    return Result && V8::BooleanValue(Panorama::Isolate(), Result);
}

std::string Panel::Id() const
{
    V8::Local Object = Handle();
    if (!Object)
        return std::string();

    V8::Local Value = V8::Get(Panorama::Isolate(), Object, Panorama::Context(), "id");
    return Value ? V8::ToString(Panorama::Isolate(), Value) : std::string();
}

CUIPanel* Panel::Native() const
{
    V8::Local Object = Handle();
    if (!Object || V8::InternalFieldCount(Object) != Offsets::PanelObject::InternalFieldCount)
        return nullptr;
    return static_cast<CUIPanel*>(V8::ExternalValue(V8::GetInternalField(Object, Offsets::PanelObject::UIPanelField)));
}

void Panel::AddOption(const Panel& Option) const
{
    V8::Local Args[] = { Option.Handle() };
    if (Args[0])
        Panorama::Invoke(Handle(), "AddOption", 1, Args);
}

void Panel::SetSelected(const char* Id) const
{
    V8::Local Args[] = { Panorama::String(Id) };
    Panorama::Invoke(Handle(), "SetSelected", 1, Args);
}

Panel Panel::Selected() const
{
    return Panel(Panorama::Invoke(Handle(), "GetSelected"));
}

bool Panel::IsValid() const
{
    V8::Local Result = Panorama::Invoke(Handle(), "IsValid");
    return Result && V8::BooleanValue(Panorama::Isolate(), Result);
}

void Panel::Release()
{
    if (Global)
        V8::DisposeGlobal(Global);
    Global = nullptr;
}

void Panel::Delete()
{
    V8::Local Args[] = { Panorama::Number(0.0) };
    Panorama::Invoke(Handle(), "DeleteAsync", 1, Args);
    Release();
}

void Panel::SetStyle(const char* Property, const char* Value) const
{
    V8::Local Object = Handle();
    if (!Object)
        return;

    V8::Local StyleObject = V8::Get(Panorama::Isolate(), Object, Panorama::Context(), "style");
    if (StyleObject && V8::IsObject(StyleObject))
        V8::Set(StyleObject, Panorama::Context(), Panorama::String(Property), Panorama::String(Value));
}

void Panel::SetStyle(Style Styles) const
{
    V8::Local Object = Handle();
    if (!Object)
        return;

    V8::Local StyleObject = V8::Get(Panorama::Isolate(), Object, Panorama::Context(), "style");
    if (!StyleObject || !V8::IsObject(StyleObject))
        return;

    for (const auto& [Property, Value] : Styles)
        V8::Set(StyleObject, Panorama::Context(), Panorama::String(Property), Panorama::String(Value));
}

void Panel::SetText(const char* Text) const
{
    V8::Local Object = Handle();
    if (Object)
        V8::Set(Object, Panorama::Context(), Panorama::String("text"), Panorama::String(Text));
}

void Panel::SetImage(const char* Path) const
{
    V8::Local Args[] = { Panorama::String(Path) };
    Panorama::Invoke(Handle(), "SetImage", 1, Args);
}

void Panel::AddClass(const char* Class) const
{
    V8::Local Args[] = { Panorama::String(Class) };
    Panorama::Invoke(Handle(), "AddClass", 1, Args);
}

void Panel::RemoveClass(const char* Class) const
{
    V8::Local Args[] = { Panorama::String(Class) };
    Panorama::Invoke(Handle(), "RemoveClass", 1, Args);
}

void Panel::SetVisible(bool Visible) const
{
    SetStyle("visibility", Visible ? "visible" : "collapse");
}

void Panel::SetEvent(const char* Event, Panorama::Handler Callback, std::intptr_t Argument) const
{
    V8::Local Args[] = { Panorama::String(Event), Panorama::Function(Callback, Argument) };
    if (Args[1])
        Panorama::Invoke(Handle(), "SetPanelEvent", 2, Args);
}

V8::Local Panel::Handle() const
{
    if (!Global || !*Global || !Panorama::Isolate())
        return nullptr;
    return V8::CreateHandle(Panorama::Isolate(), *Global);
}
