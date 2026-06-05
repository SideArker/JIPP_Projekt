#include "AIController.hpp"
#include "CameraController.hpp"
#include "FileManager.hpp"
#include "GameContent.hpp"
#include "MapFile.hpp"
#include "MapManager.hpp"
#include "ProductionUI.hpp"
#include "RightPanelUI.hpp"
#include "SoundManager.hpp"
#include "TerrainMovement.hpp"
#include "TextureManager.hpp"
#include "Unit.hpp"
#include <SFML/Graphics.hpp>
#include <map>
#include <memory>
#include <optional>

static constexpr const char *LEVEL1_PATH = "Art/levels/level1.map";

int main() {
  sf::RenderWindow window(sf::VideoMode({1280, 720}), "Map Renderer",
                          sf::State::Windowed);
  sf::View view(sf::FloatRect({0.f, 0.f}, {640.f, 360.f}));

  window.setView(view);
  GameContent::init();
  tgui::Texture::setDefaultSmooth(false);
  tgui::ToolTip::setInitialDelay(std::chrono::milliseconds(0));

  MapManager mapManager;
  GameContent::configure(mapManager);
  if (!mapManager.loadFromFile(LEVEL1_PATH))
    return -1;
  mapManager.setupInput(window);

  std::unique_ptr<ProductionUI> productionUI;
  if (tgui::Gui *gui = mapManager.getGui()) {
    productionUI = std::make_unique<ProductionUI>(
        *gui, mapManager, mapManager.getTurnController());
    mapManager.setOnOpenFactory(
        [&productionUI](std::shared_ptr<Building> factory) {
          if (productionUI)
            productionUI->open(factory);
        });
  }

  view.setSize({515.f, 285.f});
  view.setCenter({257.5f, 142.5f});
  view.setViewport(sf::FloatRect({0.f, 0.f}, {1030.f / 1280.f, 570.f / 720.f}));

  CameraController cameraController;
  sf::Vector2u mapPixels(mapManager.getMapWidth() * mapManager.getTileSize().x,
                         mapManager.getMapHeight() *
                             mapManager.getTileSize().y);
  cameraController.init(view, mapPixels, window.getSize());

  GameUIWidgets panels;
  std::map<Team, tgui::Label::Ptr> teamMoneyLabels;
  if (tgui::Gui *gui = mapManager.getGui()) {
    gui->setFont("Art/Fonts/joystixMonospace.ttf");
    panels = buildGameUI(*gui);
    panels.endTurnBtn->onClick(
        [&mapManager]() { mapManager.requestEndTurn(); });

    panels.undoBtn->onClick([&mapManager, &cameraController]() {
      std::shared_ptr<Unit> outUnit;
      if (mapManager.popUndoState(outUnit)) {
        if (outUnit) {
          cameraController.trackUnit(outUnit);
          mapManager.selectUnit(outUnit);
        }
      }
    });

    panels.nextUnitBtn->onClick([&mapManager, &cameraController]() {
      auto &units = mapManager.getUnits();
      std::vector<std::shared_ptr<Unit>> available;
      for (auto &u : units) {
        if (!u->isDead() && u->getTeam() == Team::Ally && !u->hasActed())
          available.push_back(u);
      }
      if (!available.empty()) {
        int idx = rand() % available.size();
        cameraController.trackUnit(available[idx]);
        mapManager.selectUnit(available[idx]);
      }
    });

    panels.settingsBtn->onClick([&panels]() {
      panels.settingsPanel->setVisible(!panels.settingsPanel->isVisible());
    });
    panels.closeSettingsBtn->onClick(
        [&panels]() { panels.settingsPanel->setVisible(false); });
    panels.saveGameBtn->onClick(
        [&mapManager]() { mapManager.saveToFile("savegame.sav"); });
    panels.musicVolSlider->onValueChange(
        [](float v) { SoundManager::setMusicVolume(v); });
    panels.soundVolSlider->onValueChange(
        [](float v) { SoundManager::setSFXVolume(v); });

    panels.teamList->removeAllWidgets();
    int teamY = 0;
    for (const auto &[team, data] : mapManager.getTeams()) {
      if (team == Team::Neutral)
        continue;

      auto teamPanel = tgui::Panel::create({"100%", 45});
      teamPanel->setPosition(0, teamY);
      teamPanel->getRenderer()->setBackgroundColor(sf::Color(30, 30, 35, 180));
      panels.teamList->add(teamPanel);

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
      teamMoneyLabels[team] = moneyLbl;

      teamY += 50;
    }

    mapManager.onUnitMoveStart = [&cameraController](std::shared_ptr<Unit> u) {
      cameraController.trackUnit(u);
    };

    mapManager.setOnSelectionChanged(
        [infoLabel = panels.infoLabel, flagsList = panels.flagsList](
            std::shared_ptr<Unit> unit, std::shared_ptr<Building> building,
            const Tile *tile) {
          flagsList->removeAllWidgets();
          if (unit) {
            std::string desc = unit->getName() + "\n";
            desc += "Type: Unit\n";
            desc += "HP: " + std::to_string(unit->getHealth()) + "/" +
                    std::to_string(unit->getMaxHealth()) + "\n";
            desc += "Attack: " + std::to_string(unit->getDamage());
            infoLabel->setText(desc);

            int flagY = 10;
            if (unit->hasFlag(UnitFlag::Capture)) {
              tgui::Texture tex("Art/UI/flag.png",
                                tgui::UIntRect(0, 0, 32, 32));
              auto pic = tgui::Picture::create(tex);
              pic->setPosition(10, flagY);
              auto tooltip = tgui::Label::create("Can capture buildings");
              tooltip->getRenderer()->setBackgroundColor(
                  sf::Color(40, 40, 45, 230));
              tooltip->getRenderer()->setTextColor(sf::Color::White);
              tooltip->setTextSize(14);
              pic->setToolTip(tooltip);
              flagsList->add(pic);
              flagY += 40;
            }
          } else if (building) {
            infoLabel->setText(building->getTypeName() +
                               "\n\nHP: --/--\nAttack: --");
          } else {
            infoLabel->setText("");
          }
        });
  }

  AIController aiController;
  sf::Clock clock;

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
        break;
      }
      if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape && productionUI &&
            productionUI->isOpen()) {
          productionUI->close();
          continue;
        }
      }
      mapManager.handleEvent(*event);
    }

    if (!window.isOpen())
      break;

    float deltaTime = clock.restart().asSeconds();

    if (mapManager.getCurrentTeam() == Team::Enemy) {
      if (!mapManager.isAnyUnitActing()) {
        aiController.update(deltaTime, mapManager,
                            mapManager.getTurnController(), cameraController);
      }
      if (aiController.isDone() && !mapManager.isAnyUnitActing()) {
        mapManager.endTurn();

        auto &units = mapManager.getUnits();
        std::vector<std::shared_ptr<Unit>> allies;
        for (auto &u : units) {
          if (!u->isDead() && u->getTeam() == Team::Ally)
            allies.push_back(u);
        }
        if (!allies.empty()) {
          int idx = rand() % allies.size();
          cameraController.trackUnit(allies[idx]);
        }
      }
    } else {
      cameraController.release();
      aiController.reset();
    }

    mapManager.update(deltaTime);
    if (productionUI)
      productionUI->update(deltaTime);
    cameraController.update(deltaTime, window);
    mapManager.syncCameraView(cameraController.getView());

    if (mapManager.getGui()) {
      for (auto &[t, lbl] : teamMoneyLabels) {
        auto it = mapManager.getTeams().find(t);
        if (it != mapManager.getTeams().end())
          lbl->setText("$" + std::to_string(it->second.money));
      }
      panels.undoBtn->setEnabled(!mapManager.isUndoStackEmpty() &&
                                 !mapManager.isAnyUnitActing());
      bool hasNext = false;
      for (auto &u : mapManager.getUnits()) {
        if (!u->isDead() && u->getTeam() == Team::Ally && !u->hasActed()) {
          hasNext = true;
          break;
        }
      }
      panels.nextUnitBtn->setEnabled(hasNext && !mapManager.isAnyUnitActing());

      if (mapManager.getCurrentTeam() == Team::Enemy) {
        panels.endTurnBtn->setText("Enemy Turn");
        panels.endTurnBtn->setEnabled(false);
      } else {
        panels.endTurnBtn->setText("End Turn >>");
        panels.endTurnBtn->setEnabled(!mapManager.isAnyUnitActing());
      }
    }

    window.clear();
    window.setView(cameraController.getView());
    mapManager.draw(window);

    window.setView(view);
    mapManager.drawUI(window);
    window.display();
  }

  SoundManager::shutdown();
  TextureManager::clearCache();
  return 0;
}
