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
    btn->getRenderer()->setTextColor(sf::Color(200, 220, 255));
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
  result.endTurnBtn->getRenderer()->setTextColor(sf::Color(255, 200, 200));
  result.rightPanel->add(result.endTurnBtn);

  result.bottomPanel = tgui::Panel::create();
  result.bottomPanel->setSize("100% - 250px", "150px");
  result.bottomPanel->setPosition("0", "100% - 150px");
  result.bottomPanel->getRenderer()->setBackgroundColor(sf::Color(15, 25, 35));
  result.bottomPanel->getRenderer()->setBorders(tgui::Borders(0, 2, 0, 0));
  result.bottomPanel->getRenderer()->setBorderColor(sf::Color(60, 85, 115));
  gui.add(result.bottomPanel);

  result.infoLabel = tgui::Label::create(
      "Selection_Name\nDescription here\nHP: --/--\nAttack: --");
  result.infoLabel->setPosition("20px", "20px");
  result.infoLabel->getRenderer()->setTextColor(sf::Color(200, 220, 255));
  result.infoLabel->setTextSize(16);
  result.bottomPanel->add(result.infoLabel);

  result.flagsList = tgui::ScrollablePanel::create();
  result.flagsList->setSize("200px", "100%");
  result.flagsList->setPosition("100% - 200px", "0px");
  result.flagsList->getRenderer()->setBackgroundColor(sf::Color::Transparent);
  result.flagsList->getRenderer()->setScrollbarWidth(0); // hidden scrollbar if any
  result.bottomPanel->add(result.flagsList);

  return result;
}
