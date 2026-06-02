#pragma once

#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <vector>

struct RightPanelWidgets {
    std::vector<tgui::Panel::Ptr> infoBoxes;
    tgui::Button::Ptr undoBtn;
    tgui::Button::Ptr nextUnitBtn;
    tgui::Button::Ptr settingsBtn;
    tgui::Button::Ptr endTurnBtn;
};

RightPanelWidgets buildRightPanelUI(
    tgui::Gui& gui,
    float panelOriginXGU,
    float panelOriginYGU,
    float scaleX,
    float scaleY,
    int   numBoxes,
    float boxHeightGU,
    float btnHeightGU     = 18.f,
    float endTurnHeightGU = 22.f
);
