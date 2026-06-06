#include "RightPanelUI.hpp"

GameUIWidgets buildGameUI(tgui::Gui &gui) {
  GameUIWidgets result;

  auto createBorder = [&](const char *w, const char *h, const char *x,
                          const char *y) {
    auto border = tgui::Panel::create();
    border->setSize(w, h);
    border->setPosition(x, y);
    border->getRenderer()->setBackgroundColor(sf::Color(40, 60, 80, 150));
    gui.add(border);
    return border;
  };
  result.screenOverlay = createBorder("100% - 250px", "2px", "0", "0"); // Top
  createBorder("100% - 250px", "2px", "0", "100% - 152px"); // Bottom
  createBorder("2px", "100% - 150px", "0", "0");            // Left
  createBorder("2px", "100% - 150px", "100% - 252px", "0"); // Right

  result.rightPanel = tgui::Panel::create();
  result.rightPanel->setSize("250px", "100%");
  result.rightPanel->setPosition("100% - 250px", "0");
  result.rightPanel->getRenderer()->setBackgroundColor(sf::Color(15, 25, 35));
  result.rightPanel->getRenderer()->setBorders(tgui::Borders(2, 0, 0, 0));
  result.rightPanel->getRenderer()->setBorderColor(sf::Color(60, 85, 115));
  gui.add(result.rightPanel);

  result.teamList = tgui::ScrollablePanel::create();
  result.teamList->setSize("100% - 20px", "35%");
  result.teamList->setPosition("10px", "10px");
  result.teamList->getRenderer()->setBackgroundColor(sf::Color(8, 16, 28));
  result.teamList->getRenderer()->setBorders(tgui::Borders(1));
  result.teamList->getRenderer()->setBorderColor(sf::Color(55, 85, 115));
  result.rightPanel->add(result.teamList);

  auto createBtn = [](const std::string &text) {
    auto btn = tgui::Button::create(text);
    btn->getRenderer()->setBackgroundColor(sf::Color(30, 45, 60));
    btn->getRenderer()->setBackgroundColorHover(
        sf::Color(50, 75, 100)); // Lighter on hover
    btn->getRenderer()->setBackgroundColorDown(sf::Color(20, 30, 40));
    btn->getRenderer()->setBackgroundColorDisabled(sf::Color(20, 25, 30)); // Darker background
    btn->getRenderer()->setTextColor(sf::Color(200, 220, 255));
    btn->getRenderer()->setTextColorDisabled(sf::Color(100, 110, 130)); // Darker text
    btn->getRenderer()->setBorders(tgui::Borders(1));
    btn->getRenderer()->setBorderColor(sf::Color(60, 85, 115));
    return btn;
  };

  result.undoBtn = createBtn("Undo Move");
  result.undoBtn->setSize("100% - 40px", "40px");
  result.undoBtn->setPosition("20px", "100% - 260px");
  result.rightPanel->add(result.undoBtn);

  result.nextUnitBtn = createBtn("Next Unit");
  result.nextUnitBtn->setSize("100% - 40px", "40px");
  result.nextUnitBtn->setPosition("20px", "100% - 210px");
  result.rightPanel->add(result.nextUnitBtn);

  result.settingsBtn = createBtn("Settings");
  result.settingsBtn->setSize("100% - 40px", "40px");
  result.settingsBtn->setPosition("20px", "100% - 160px");
  result.rightPanel->add(result.settingsBtn);

  result.endTurnBtn = createBtn("End Turn >>");
  result.endTurnBtn->setSize("100% - 20px", "70px"); // Made bigger
  result.endTurnBtn->setPosition("10px", "100% - 80px");
  result.endTurnBtn->getRenderer()->setBackgroundColor(sf::Color(70, 35, 35));
  result.endTurnBtn->getRenderer()->setBackgroundColorHover(
      sf::Color(100, 50, 50));
  result.endTurnBtn->getRenderer()->setBackgroundColorDisabled(sf::Color(35, 20, 20));
  result.endTurnBtn->getRenderer()->setTextColor(sf::Color(255, 200, 200));
  result.endTurnBtn->getRenderer()->setTextColorDisabled(sf::Color(130, 100, 100));
  result.rightPanel->add(result.endTurnBtn);

  result.bottomPanel = tgui::Panel::create();
  result.bottomPanel->setSize("100% - 250px", "150px");
  result.bottomPanel->setPosition("0", "100% - 150px");
  result.bottomPanel->getRenderer()->setBackgroundColor(sf::Color(15, 25, 35));
  result.bottomPanel->getRenderer()->setBorders(tgui::Borders(0, 2, 0, 0));
  result.bottomPanel->getRenderer()->setBorderColor(sf::Color(60, 85, 115));
  gui.add(result.bottomPanel);

  result.unitPortrait = tgui::Picture::create();
  result.unitPortrait->setSize("64px", "64px");
  result.unitPortrait->setPosition("20px", "20px");
  result.unitPortrait->setVisible(false);
  result.bottomPanel->add(result.unitPortrait);

  result.infoLabel = tgui::Label::create("");
  result.infoLabel->setPosition("100px", "20px");
  result.infoLabel->getRenderer()->setTextColor(sf::Color(200, 220, 255));
  result.infoLabel->setTextSize(16);
  result.bottomPanel->add(result.infoLabel);

  result.flagsList = tgui::ScrollablePanel::create();
  result.flagsList->setSize("200px", "100%");
  result.flagsList->setPosition("100% - 200px", "0px");
  result.flagsList->getRenderer()->setBackgroundColor(sf::Color(25, 35, 45));
  result.flagsList->getRenderer()->setBorders(tgui::Borders(1, 0, 0, 0));
  result.flagsList->getRenderer()->setBorderColor(sf::Color(60, 85, 115));
  result.flagsList->getRenderer()->setScrollbarWidth(0); // hidden scrollbar if any
  result.bottomPanel->add(result.flagsList);

  buildSettingsPanel(gui, &result);

  return result;
}

tgui::Panel::Ptr buildSettingsPanel(tgui::Gui& gui, GameUIWidgets* outWidgets) {
  auto panel = tgui::Panel::create();
  panel->setSize("400px", "320px");
  panel->setPosition("50% - 200px", "50% - 160px");
  panel->getRenderer()->setBackgroundColor(sf::Color(25, 35, 45, 240));
  panel->getRenderer()->setBorders(tgui::Borders(2));
  panel->getRenderer()->setBorderColor(sf::Color(60, 85, 115));
  panel->setVisible(false); // Hidden by default
  gui.add(panel);

  auto title = tgui::Label::create("Settings");
  title->setPosition("50% - 40px", "10px");
  title->setTextSize(20);
  title->getRenderer()->setTextColor(sf::Color::White);
  panel->add(title);

  auto createLabel = [](const std::string& txt, const std::string& y) {
      auto lbl = tgui::Label::create(txt);
      lbl->setPosition("20px", y.c_str());
      lbl->getRenderer()->setTextColor(sf::Color::White);
      return lbl;
  };
  
  panel->add(createLabel("Music Volume:", "60px"));
  auto musicSlider = tgui::Slider::create(0, 100);
  musicSlider->setPosition("150px", "60px");
  musicSlider->setSize("200px", "16px");
  musicSlider->setValue(20);
  panel->add(musicSlider);

  panel->add(createLabel("Sound Volume:", "100px"));
  auto soundSlider = tgui::Slider::create(0, 100);
  soundSlider->setPosition("150px", "100px");
  soundSlider->setSize("200px", "16px");
  soundSlider->setValue(100);
  panel->add(soundSlider);

  panel->add(createLabel("Fullscreen:", "140px"));
  auto fullscreenCheckbox = tgui::CheckBox::create();
  fullscreenCheckbox->setPosition("150px", "140px");
  panel->add(fullscreenCheckbox);

  auto saveBtn = tgui::Button::create("Save Game");
  saveBtn->setPosition("20px", "180px");
  saveBtn->setSize("360px", "40px");
  panel->add(saveBtn);

  auto quitBtn = tgui::Button::create("Quit to Title");
  quitBtn->setPosition("20px", "240px");
  quitBtn->setSize("170px", "40px");
  panel->add(quitBtn);

  auto closeBtn = tgui::Button::create("Close");
  closeBtn->setPosition("210px", "240px");
  closeBtn->setSize("170px", "40px");
  panel->add(closeBtn);

  if (outWidgets) {
      outWidgets->settingsPanel = panel;
      outWidgets->musicVolSlider = musicSlider;
      outWidgets->soundVolSlider = soundSlider;
      outWidgets->fullscreenCheckbox = fullscreenCheckbox;
      outWidgets->saveGameBtn = saveBtn;
      outWidgets->quitBtn = quitBtn;
      outWidgets->closeSettingsBtn = closeBtn;
  }

  return panel;
}
