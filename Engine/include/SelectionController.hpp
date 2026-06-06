#pragma once

#include "SelectionState.hpp"
#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <vector>


class MapManager;
class Unit;
class TurnController;
class Building;

class SelectionController {
public:
  SelectionController(sf::RenderWindow &window, MapManager &mapManager,
                      TurnController &turnController,
                      const std::string &walkOverlayPath,
                      const std::string &moveArrowPath,
                      const std::string &iconsPath,
                      const std::string &enemyOverlayPath,
                      const std::string &friendlyOverlayPath);
  void handleEvent(const sf::Event &event);
  void drawOverlays(sf::RenderTarget &target);
  void drawCursorIcon(sf::RenderTarget &target);
  void clearSelection();
  void selectUnit(std::shared_ptr<Unit> unit);
  void syncCameraView(sf::View gameView);
  void update(float dt);
  void setOnOpenFactory(std::function<void(std::shared_ptr<Building>)> cb) {
    m_onOpenFactory = std::move(cb);
  }
  std::shared_ptr<Unit> getSelectedUnit() const { return selectedUnit; }

private:
  void openFactoryUI(std::shared_ptr<Building> factory);
  void updateAttackPath(sf::Vector2i enemyGrid, sf::Vector2i unitGrid,
                        sf::Vector2i preferredDir);

  std::function<void(std::shared_ptr<Building>)> m_onOpenFactory;

  SelectionState m_state = SelectionState::Idle;
  sf::RenderWindow &m_window;
  MapManager &mapManager;
  TurnController &m_turnController;
  std::shared_ptr<Unit> selectedUnit;
  std::shared_ptr<Unit> hoveredEnemyUnit;
  std::vector<sf::Vector2i> reachableTiles;
  std::vector<sf::Vector2i> previewPath;
  sf::Texture m_walkOverlayTexture;
  sf::Texture m_moveArrowTexture;
  sf::Texture m_iconsTexture;
  sf::Texture m_enemyOverlayTexture;
  sf::Texture m_friendlyOverlayTexture;
  sf::Texture m_unit_overlay_enemy;
  sf::View m_gameView;
  sf::Vector2f m_cursorPos;
  sf::Vector2f m_screenCursorPos;
  int m_cursorIconCell{-1};
  sf::Vector2i m_preferredApproachDir{0, 0};
  float m_scaleX{1.f};
  float m_scaleY{1.f};

  void handleMouseMoved(sf::Vector2i gridPos, sf::Vector2f localPos);
  void handleMouseClicked(sf::Vector2i gridPos);
};
