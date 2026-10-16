#pragma once

#include "SDK/Panorama/CUIPanel.h"
#include "SDK/Panorama/Panorama.h"
#include "SDK/V8/V8.h"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

class Panel
{
public:
    using Style = std::initializer_list<std::pair<const char*, const char*>>;

    Panel() = default;

    static Panel Root();
    static Panel Create(const char* Type, const Panel& Parent, const char* Id);
    static Panel Create(const char* Type, const Panel& Parent, const char* Id, Style Styles);

    Panel FindChild(const char* Id) const;
    Panel FindChildTraverse(const char* Id) const;
    std::vector<Panel> FindChildrenWithClass(const char* Class) const;
    std::vector<Panel> Children() const;
    Panel Parent() const;
    Panel FindAncestor(const char* Id) const;
    std::string Id() const;
    std::string Type() const;
    bool HasClass(const char* Class) const;
    double Number(const char* Property) const;
    bool PositionWithin(const Panel& Ancestor, double& X, double& Y) const;
    CUIPanel* Native() const;

    void AddOption(const Panel& Option) const;
    void SetSelected(const char* Id) const;
    Panel Selected() const;

    bool IsValid() const;
    void Release();
    void Delete();

    void SetStyle(const char* Property, const char* Value) const;
    void SetStyle(Style Styles) const;
    void SetText(const char* Text) const;
    void SetImage(const char* Path) const;
    void AddClass(const char* Class) const;
    void RemoveClass(const char* Class) const;
    void SetVisible(bool Visible) const;
    void SetEvent(const char* Event, Panorama::Handler Callback, std::intptr_t Argument) const;

    V8::Local Handle() const;

private:
    explicit Panel(V8::Local Object);
    static std::vector<Panel> FromArray(V8::Local Array);

    V8::Address* Global = nullptr;
};
