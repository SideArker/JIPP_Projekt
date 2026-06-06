#include "AIController.hpp"
#include "CameraController.hpp"
#include "EconomyManager.hpp"
#include "FileManager.hpp"
#include "GameContent.hpp"
#include "GameUIManager.hpp"
#include "MapFile.hpp"
#include "MapManager.hpp"
#include "SoundManager.hpp"
#include "TerrainMovement.hpp"
#include "TextureManager.hpp"
#include "Unit.hpp"
#include <SFML/Graphics.hpp>
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
  mapManager.onTurnEnded = [&mapManager]() {
    EconomyManager::processTurnEnd(mapManager);
  };

  view.setSize({515.f, 285.f});
  view.setCenter({257.5f, 142.5f});
  view.setViewport(sf::FloatRect({0.f, 0.f}, {1030.f / 1280.f, 570.f / 720.f}));

  CameraController cameraController;
  sf::Vector2u mapPixels(mapManager.getMapWidth() * mapManager.getTileSize().x,
                         mapManager.getMapHeight() *
                             mapManager.getTileSize().y);
  cameraController.init(view, mapPixels, window.getSize());

  mapManager.onCapturePan = [&](std::shared_ptr<Unit> u) {
    cameraController.trackUnit(u);
  };

  GameUIManager uiManager(window, mapManager, cameraController);

  AIController aiController;
  sf::Clock clock;

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
        break;
      }
      if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape &&
            uiManager.getProductionUI() &&
            uiManager.getProductionUI()->isOpen()) {
          uiManager.getProductionUI()->close();
          continue;
        }
      }
      uiManager.handleEvent(*event);
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
      aiController.reset();
    }

    mapManager.update(deltaTime);
    uiManager.update(deltaTime, mapManager);
    cameraController.update(deltaTime, window);
    mapManager.syncCameraView(cameraController.getView());

    window.clear();

    window.setView(cameraController.getView());
    mapManager.draw(window);

    window.setView(window.getDefaultView());
    mapManager.drawUI(window);
    uiManager.draw();

    window.display();
  }

  mapManager.setOnSelectionChanged(nullptr);
  mapManager.setOnOpenFactory(nullptr);
  mapManager.onUnitMoveStart = nullptr;

  SoundManager::shutdown();
  TextureManager::clearCache();
  return 0;
}
