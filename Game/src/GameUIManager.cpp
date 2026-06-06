#include "GameUIManager.hpp"
#include "SettingsManager.hpp"
#include "SoundManager.hpp"
#include <SFML/Window/Context.hpp>
#include <cstdlib>

GameUIManager::GameUIManager(sf::RenderWindow &window, MapManager &mapManager,
                             CameraController &cameraController)
    : m_gui(window) {
  m_gui.setFont("Art/Fonts/joystixMonospace.ttf");
  m_panels = buildGameUI(m_gui);

  m_productionUI = std::make_unique<ProductionUI>(
      m_gui, mapManager, mapManager.getTurnController());

  mapManager.setOnOpenFactory([this](std::shared_ptr<Building> factory) {
    if (m_productionUI) {
      m_productionUI->open(factory);
    }
  });

  m_panels.endTurnBtn->onClick(
      [&mapManager]() { mapManager.requestEndTurn(); });

  m_panels.undoBtn->onClick([&mapManager, &cameraController]() {
    std::shared_ptr<Unit> outUnit;
    if (mapManager.popUndoState(outUnit)) {
      if (outUnit) {
        cameraController.trackUnit(outUnit);
        mapManager.selectUnit(outUnit);
      }
    }
  });

  m_panels.nextUnitBtn->onClick([&mapManager, &cameraController]() {
    auto &units = mapManager.getUnits();
    std::vector<std::shared_ptr<Unit>> available;
    for (auto &u : units) {
      if (!u->isDead() && u->getTeam() == mapManager.getCurrentTeam() &&
          !u->hasActed())
        available.push_back(u);
    }
    if (!available.empty()) {
      int idx = rand() % available.size();
      cameraController.trackUnit(available[idx]);
      mapManager.selectUnit(available[idx]);
    }
  });

  m_panels.settingsBtn->onClick([this]() {
    m_panels.settingsPanel->setVisible(!m_panels.settingsPanel->isVisible());
  });

  m_panels.closeSettingsBtn->onClick(
      [this]() { m_panels.settingsPanel->setVisible(false); });

  m_panels.saveGameBtn->onClick(
      [&mapManager]() { mapManager.saveToFile("savegame.sav"); });

  m_panels.quitBtn->onClick([this]() { m_quitToMenu = true; });

  Settings settings = SettingsManager::load();
  m_panels.musicVolSlider->setValue(settings.musicVolume);
  m_panels.soundVolSlider->setValue(settings.soundVolume);
  m_panels.fullscreenCheckbox->setChecked(settings.fullscreen);

  m_panels.musicVolSlider->onValueChange([](float v) {
    SoundManager::setMusicVolume(v);
    Settings s = SettingsManager::load();
    s.musicVolume = static_cast<int>(v);
    SettingsManager::save(s);
  });
  m_panels.soundVolSlider->onValueChange([](float v) {
    SoundManager::setSFXVolume(v);
    Settings s = SettingsManager::load();
    s.soundVolume = static_cast<int>(v);
    SettingsManager::save(s);
  });
  m_panels.fullscreenCheckbox->onChange([this, &window](bool checked) {
    Settings s = SettingsManager::load();
    s.fullscreen = checked;
    SettingsManager::save(s);

    sf::Context context;
    auto style = checked ? sf::State::Fullscreen : sf::State::Windowed;
    window.create(sf::VideoMode({1280, 720}), "Map Renderer", style);
    sf::Image icon;
    if (icon.loadFromFile("Art/ico.png")) {
      window.setIcon({icon.getSize().x, icon.getSize().y}, icon.getPixelsPtr());
    }
    m_gui.setWindow(window);
  });

  m_panels.teamList->removeAllWidgets();
  int teamY = 0;
  for (const auto &[team, data] : mapManager.getTeams()) {
    if (team == Team::Neutral)
      continue;

    auto teamPanel = tgui::Panel::create({"100%", 45});
    teamPanel->setPosition(0, teamY);
    teamPanel->getRenderer()->setBackgroundColor(sf::Color(30, 30, 35, 180));
    m_panels.teamList->add(teamPanel);

    auto colorStrip = tgui::Panel::create({5, "100%"});
    colorStrip->setPosition(0, 0);
    colorStrip->getRenderer()->setBackgroundColor(data.color);
    teamPanel->add(colorStrip);

    auto nameLbl = tgui::Label::create();
    nameLbl->setText(data.name);
    nameLbl->getRenderer()->setTextColor(sf::Color::White);
    nameLbl->setTextSize(14);
    nameLbl->setMaximumTextWidth(140.f);
    nameLbl->setPosition(12, "50% - 15");
    teamPanel->add(nameLbl);

    auto moneyLbl = tgui::Label::create();
    moneyLbl->setText("$" + std::to_string(data.money));
    moneyLbl->getRenderer()->setTextColor(sf::Color(220, 220, 220));
    moneyLbl->setTextSize(14);
    moneyLbl->setPosition("100% - width - 10", "50% - 7");
    teamPanel->add(moneyLbl);
    m_teamMoneyLabels[team] = moneyLbl;

    teamY += 50;
  }

  mapManager.onUnitMoveStart = [&cameraController](std::shared_ptr<Unit> u) {
    cameraController.trackUnit(u);
  };

  mapManager.setOnSelectionChanged([this, &mapManager,
                                    infoLabel = m_panels.infoLabel,
                                    flagsList = m_panels.flagsList,
                                    portrait = m_panels.unitPortrait](
                                       std::shared_ptr<Unit> unit,
                                       std::shared_ptr<Building> building,
                                       const Tile *tile) {
    flagsList->removeAllWidgets();
    portrait->setVisible(false);
    if (unit) {
      std::string desc = unit->getName() + "\n";
      desc += "HP: " + std::to_string(unit->getHealth()) + "/" +
              std::to_string(unit->getMaxHealth()) + "\n";
      desc += "Damage: " + std::to_string(unit->getDamage()) + "\n";
      desc += "Speed: " + std::to_string(unit->getMoveSpeed()) + "\n";
      desc += "Range: " + std::to_string(unit->getMinAttackRange()) + "-" +
              std::to_string(unit->getMaxAttackRange()) + "\n";
      infoLabel->setText(desc);

      try {
        tgui::Texture tex(unit->getArtPath(), tgui::UIntRect(0, 0, 32, 32));
        portrait->getRenderer()->setTexture(tex);
        portrait->setVisible(true);
      } catch (...) {
      }

      int flagY = 0;
      auto addFlagUi = [&](const std::string &tooltipText) {
        auto flagPanel = tgui::Panel::create({"100%", "36px"});
        flagPanel->setPosition(0, flagY);
        flagPanel->getRenderer()->setBackgroundColor(sf::Color::Transparent);
        tgui::Texture tex("Art/UI/flag.png", tgui::UIntRect(0, 0, 32, 32));
        auto pic = tgui::Picture::create(tex);
        pic->setSize("32px", "32px");
        pic->setPosition("2px", "2px");
        auto tooltip = tgui::Label::create(tooltipText);
        tooltip->getRenderer()->setBackgroundColor(sf::Color(40, 40, 45, 230));
        tooltip->getRenderer()->setTextColor(sf::Color::White);
        tooltip->setTextSize(14);
        pic->setToolTip(tooltip);
        flagPanel->add(pic);
        flagsList->add(flagPanel);
        flagY += 40;
      };

      if (unit->hasFlag(UnitFlag::Capture))
        addFlagUi("Can capture buildings");
    } else if (building) {
      std::string desc = building->getTypeName() + "\n";
      desc += building->getDescription() + "\n";
      auto bTd = mapManager.getTeamData(building->getTeam());
      desc += "Team: " + std::string(bTd ? bTd->name : "Neutral") + "\n";
      infoLabel->setText(desc);
    } else {
      infoLabel->setText("No selection");
    }
  });

  m_victoryPanel = tgui::Panel::create();
  m_victoryPanel->setSize("60%", "60%");
  m_victoryPanel->setPosition("50% - width / 2", "50% - height / 2");
  m_victoryPanel->getRenderer()->setBackgroundColor(sf::Color(20, 20, 25, 230));
  m_victoryPanel->getRenderer()->setBorders(2);
  m_victoryPanel->getRenderer()->setBorderColor(sf::Color::Yellow);
  m_victoryPanel->setVisible(false);

  auto vicTitle = tgui::Label::create("VICTORY");
  vicTitle->setPosition(0, "20%");
  vicTitle->setSize("100%", 100);
  vicTitle->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
  vicTitle->setTextSize(48);
  vicTitle->getRenderer()->setTextColor(sf::Color::Yellow);
  m_victoryPanel->add(vicTitle, "Title");

  auto backBtn = tgui::Button::create("Back to Menu");
  backBtn->setPosition("50% - width / 2", "70%");
  backBtn->setSize(240, 50);
  backBtn->setTextSize(24);
  backBtn->onClick([this]() { m_quitToMenu = true; });
  m_victoryPanel->add(backBtn);

  m_gui.add(m_victoryPanel);
}

void GameUIManager::handleEvent(const sf::Event &event) {
  m_gui.handleEvent(event);
}

void GameUIManager::update(float dt, MapManager &mapManager) {
  if (mapManager.isGameOver()) {
    if (!m_victoryPlayed) {
      m_victoryPlayed = true;
      m_victoryPanel->setVisible(true);

      auto wTd = mapManager.getTeamData(mapManager.getWinner());
      if (!wTd || wTd->isAi || mapManager.getWinner() == Team::Neutral) {
        m_victoryPanel->get<tgui::Label>("Title")->setText("DEFEAT");
        m_victoryPanel->get<tgui::Label>("Title")->getRenderer()->setTextColor(
            sf::Color::Red);
        m_victoryPanel->getRenderer()->setBorderColor(sf::Color::Red);
        SoundManager::playMusic("Defeat", false);
      } else {
        m_victoryPanel->get<tgui::Label>("Title")->setText("VICTORY");
        m_victoryPanel->get<tgui::Label>("Title")->getRenderer()->setTextColor(
            sf::Color::Yellow);
        m_victoryPanel->getRenderer()->setBorderColor(sf::Color::Yellow);
        SoundManager::playMusic("Victory", false);
      }

      m_victoryPanel->moveToFront();
    }
    return;
  }

  if (m_productionUI) {
    m_productionUI->update(dt);
  }

  for (auto &[t, lbl] : m_teamMoneyLabels) {
    auto it = mapManager.getTeams().find(t);
    if (it != mapManager.getTeams().end()) {
      lbl->setText("$" + std::to_string(it->second.money));
    }
  }

  m_panels.undoBtn->setEnabled(!mapManager.isUndoStackEmpty() &&
                               !mapManager.isAnyUnitActing());

  bool hasNext = false;
  for (auto &u : mapManager.getUnits()) {
    if (!u->isDead() && u->getTeam() == mapManager.getCurrentTeam() &&
        !u->hasActed()) {
      hasNext = true;
      break;
    }
  }
  m_panels.nextUnitBtn->setEnabled(hasNext && !mapManager.isAnyUnitActing());

  auto currentTd = mapManager.getTeamData(mapManager.getCurrentTeam());
  bool isAi = currentTd && currentTd->isAi;
  if (isAi) {
    m_panels.endTurnBtn->setText(currentTd->name + " Turn");
    m_panels.endTurnBtn->setEnabled(false);
  } else {
    m_panels.endTurnBtn->setText("End Turn >>");
    m_panels.endTurnBtn->setEnabled(!mapManager.isAnyUnitActing());
  }
}

void GameUIManager::draw() { m_gui.draw(); }
