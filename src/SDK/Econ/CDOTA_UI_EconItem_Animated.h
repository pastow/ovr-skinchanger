#pragma once

#include "SDK/Econ/CEconItemDefinition.h"
#include "SDK/Econ/C_EconItemView.h"
#include "SDK/Panorama/CUIPanel.h"

class CDOTA_UI_EconItem_Animated
{
public:
    static CDOTA_UI_EconItem_Animated* FromNative(CUIPanel* Native);

    C_EconItemView* Item() const;
    CEconItemDefinition* ItemDefinition() const;
};
