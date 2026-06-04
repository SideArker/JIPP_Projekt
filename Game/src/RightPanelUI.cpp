#include "RightPanelUI.hpp"

GameUIWidgets buildGameUI(tgui::Gui& gui) {
  GameUIWidgets result;

  // 1. Screen Overlay (Background layer for game view)
  result.screenOverlay = tgui::Panel::create();
  result.screenOverlay->setSize("100% - 250px", "100% - 150px");
  result.screenOverlay->setPosition("0", "0");
  // Simple transparent overlay, effectively darkening the edges or just providing a frame
  result.screenOverlay->getRenderer()->setBackgroundColor(sf::Color(0, 0, 0, 50));
  result.screenOverlay->getRenderer()->setBorders(tgui::Borders(0, 0, 2, 2));
  result.screenOverlay->getRenderer()->setBorderColor(sf::Color(40, 60, 80));
  gui.add(result.screenOverlay);

  // 2. Right Panel (TeamList & Action Buttons)
  result.rightPanel = tgui::Panel::create();
  result.rightPanel->setSize("250px", "100%");
  result.rightPanel->setPosition("100% - 250px", "0");
  result.rightPanel->getRenderer()->setBackgroundColor(sf::Color(15, 25, 35));
  result.rightPanel->getRenderer()->setBorders(tgui::Borders(2, 0, 0, 0));
  result.rightPanel->getRenderer()->setBorderColor(sf::Color(60, 85, 115));
  gui.add(result.rightPanel);

  // TeamList ListBox inside Right Panel
  result.teamList = tgui::ListBox::create();
  result.teamList->setSize("100% - 20px", "40%");
  result.teamList->setPosition("10px", "10px");
  result.teamList->getRenderer()->setBackgroundColor(sf::Color(8, 16, 28));
  result.teamList->getRenderer()->setTextColor(sf::Color::White);
  result.teamList->getRenderer()->setSelectedBackgroundColor(sf::Color(30, 60, 90));
  result.teamList->getRenderer()->setBorders(tgui::Borders(1));
  result.teamList->getRenderer()->setBorderColor(sf::Color(55, 85, 115));
  result.teamList->addItem("Ally Team");
  result.teamList->addItem("Enemy Team");
  result.rightPanel->add(result.teamList);

  // Helper lambda for native TGUI buttons
  auto createBtn = [](const std::string& text) {
      auto btn = tgui::Button::create(text);
      btn->getRenderer()->setBackgroundColor(sf::Color(30, 45, 60));
      btn->getRenderer()->setBackgroundColorHover(sf::Color(40, 60, 80));
      btn->getRenderer()->setBackgroundColorDown(sf::Color(20, 30, 40));
      btn->getRenderer()->setTextColor(sf::Color(200, 220, 255));
      btn->getRenderer()->setBorders(tgui::Borders(1));
      btn->getRenderer()->setBorderColor(sf::Color(60, 85, 115));
      return btn;
  };

  // Buttons inside Right Panel
  result.undoBtn = createBtn("Undo Move");
  result.undoBtn->setSize("100% - 20px", "35px");
  result.undoBtn->setPosition("10px", "100% - 190px");
  result.rightPanel->add(result.undoBtn);

  result.nextUnitBtn = createBtn("Next Unit");
  result.nextUnitBtn->setSize("100% - 20px", "35px");
  result.nextUnitBtn->setPosition("10px", "100% - 145px");
  result.rightPanel->add(result.nextUnitBtn);

  result.settingsBtn = createBtn("Settings");
  result.settingsBtn->setSize("100% - 20px", "35px");
  result.settingsBtn->setPosition("10px", "100% - 100px");
  result.rightPanel->add(result.settingsBtn);

  result.endTurnBtn = createBtn("End Turn >>");
  result.endTurnBtn->setSize("100% - 20px", "50px");
  result.endTurnBtn->setPosition("10px", "100% - 60px");
  result.endTurnBtn->getRenderer()->setBackgroundColor(sf::Color(60, 30, 30));
  result.endTurnBtn->getRenderer()->setBackgroundColorHover(sf::Color(90, 40, 40));
  result.endTurnBtn->getRenderer()->setTextColor(sf::Color(255, 200, 200));
  result.rightPanel->add(result.endTurnBtn);

  // 3. Bottom Panel (Info Tab)
  result.bottomPanel = tgui::Panel::create();
  result.bottomPanel->setSize("100% - 250px", "150px");
  result.bottomPanel->setPosition("0", "100% - 150px");
  result.bottomPanel->getRenderer()->setBackgroundColor(sf::Color(15, 25, 35));
  result.bottomPanel->getRenderer()->setBorders(tgui::Borders(0, 2, 0, 0));
  result.bottomPanel->getRenderer()->setBorderColor(sf::Color(60, 85, 115));
  gui.add(result.bottomPanel);

  result.infoLabel = tgui::Label::create("Selected Unit Stats:\nHP: 10/10\nAttack: 5\n\nTile Stats:\nGrass (Def +1)\n\nBuilding Info:\nNone");
  result.infoLabel->setPosition("20px", "20px");
  result.infoLabel->getRenderer()->setTextColor(sf::Color(200, 220, 255));
  result.infoLabel->setTextSize(14);
  result.bottomPanel->add(result.infoLabel);

  return result;
}
