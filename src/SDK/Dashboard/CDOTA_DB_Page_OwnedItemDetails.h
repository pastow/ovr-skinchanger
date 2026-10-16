#pragma once

#include "SDK/Econ/C_EconItemView.h"
#include "SDK/Panorama/Panel.h"

class CDOTA_DB_Page_OwnedItemDetails
{
public:
    static CDOTA_DB_Page_OwnedItemDetails* FromPanel(const Panel& Page);

    C_EconItemView* Item() const;
};
