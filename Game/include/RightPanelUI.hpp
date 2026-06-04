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

  tgui::Panel::Ptr settingsPanel;
  tgui::Button::Ptr closeSettingsBtn;
  tgui::Button::Ptr saveGameBtn;
  tgui::Button::Ptr quitBtn;
  tgui::Slider::Ptr musicVolSlider;
  tgui::Slider::Ptr soundVolSlider;
};

GameUIWidgets buildGameUI(tgui::Gui &gui);
tgui::Panel::Ptr buildSettingsPanel(tgui::Gui& gui, GameUIWidgets* outWidgets = nullptr);
