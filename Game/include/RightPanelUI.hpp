#pragma once

#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <vector>

struct GameUIWidgets {
    tgui::Panel::Ptr rightPanel;
    tgui::Panel::Ptr bottomPanel;
    tgui::Panel::Ptr screenOverlay;

    tgui::ListBox::Ptr teamList;
    tgui::Label::Ptr infoLabel;

    tgui::Button::Ptr undoBtn;
    tgui::Button::Ptr nextUnitBtn;
    tgui::Button::Ptr settingsBtn;
    tgui::Button::Ptr endTurnBtn;
};

GameUIWidgets buildGameUI(tgui::Gui& gui);
