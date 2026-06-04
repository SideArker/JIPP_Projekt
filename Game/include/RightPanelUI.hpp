#pragma once

#include <TGUI/Backend/SFML-Graphics.hpp>
#include <TGUI/TGUI.hpp>

struct GameUIWidgets {
  tgui::Panel::Ptr rightPanel;
  tgui::Panel::Ptr bottomPanel;
  tgui::Panel::Ptr screenOverlay;

  tgui::ScrollablePanel::Ptr teamList;
  tgui::Label::Ptr infoLabel;
  tgui::ScrollablePanel::Ptr flagsList;

  tgui::Button::Ptr undoBtn;
  tgui::Button::Ptr nextUnitBtn;
  tgui::Button::Ptr settingsBtn;
  tgui::Button::Ptr endTurnBtn;
};

GameUIWidgets buildGameUI(tgui::Gui &gui);
