#include "SelectionController.hpp"
#include "Building.hpp"
#include "MapManager.hpp"
#include "TurnController.hpp"
#include "Unit.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

static sf::Vector2i computeApproachDir(sf::Vector2f localPos, float btnW, float btnH) {
  float rx = localPos.x / btnW;
  float ry = localPos.y / btnH;

  const float deadzone = 0.28f;
  if (rx > deadzone && rx < 1.f - deadzone && ry > deadzone && ry < 1.f - deadzone)
    return {0, 0};

  if (ry < rx && ry < 1.f - rx) return {0, -1};
  if (ry > rx && ry > 1.f - rx) return {0, 1};
  if (rx <= ry && rx <= 1.f - ry) return {-1, 0};
  return {1, 0};
}

SelectionController::SelectionController(
    sf::RenderWindow &window, MapManager &mapManager,
    TurnController &turnController, const std::string &walkOverlayPath,
    const std::string &moveArrowPath, const std::string &iconsPath,
    const std::string &enemyOverlayPath, const std::string &friendlyOverlayPath)
    : m_window(window), mapManager(mapManager),
      m_turnController(turnController), selectedUnit(nullptr) {
  try {
    if (!m_walkOverlayTexture.loadFromFile(walkOverlayPath))
      throw std::runtime_error("Failed to load walk overlay texture");
    if (!m_moveArrowTexture.loadFromFile(moveArrowPath))
      throw std::runtime_error("Failed to load move arrow texture");
    if (!m_iconsTexture.loadFromFile(iconsPath))
      throw std::runtime_error("Failed to load icons texture");
    if (!enemyOverlayPath.empty() && !m_enemyOverlayTexture.loadFromFile(enemyOverlayPath))
      throw std::runtime_error("Failed to load enemy overlay texture");
    if (!friendlyOverlayPath.empty() && !m_friendlyOverlayTexture.loadFromFile(friendlyOverlayPath))
      throw std::runtime_error("Failed to load friendly overlay texture");
  } catch (const std::exception &e) {
    std::cerr << "Error loading overlay texture: " << e.what() << std::endl;
  }
}

void SelectionController::handleEvent(const sf::Event &event) {
  if (mapManager.isGameOver()) return;

  const sf::Vector2u tileSize = mapManager.getTileSize();
  const float tw = static_cast<float>(tileSize.x);
  const float th = static_cast<float>(tileSize.y);

  if (auto *mm = event.getIf<sf::Event::MouseMoved>()) {
    m_screenCursorPos = sf::Vector2f(static_cast<float>(mm->position.x),
                                     static_cast<float>(mm->position.y));
    m_cursorPos = m_window.mapPixelToCoords({mm->position.x, mm->position.y}, m_gameView);

    sf::Vector2f worldPos = m_cursorPos;
    int tileX = static_cast<int>(worldPos.x / tw);
    int tileY = static_cast<int>(worldPos.y / th);
    sf::Vector2i gridPos(tileX, tileY);

    if (tileX >= 0 && tileX < static_cast<int>(mapManager.getMapWidth()) &&
        tileY >= 0 && tileY < static_cast<int>(mapManager.getMapHeight())) {
      
      float localX = worldPos.x - tileX * tw;
      float localY = worldPos.y - tileY * th;
      handleMouseMoved(gridPos, {localX, localY});
    } else {
      handleMouseMoved({-1, -1}, {0, 0});
    }
  }

  if (const auto *kp = event.getIf<sf::Event::KeyPressed>()) {
    if ((kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::Space) &&
        m_turnController.getCurrentTeam() == Team::Ally &&
        !mapManager.isAnyUnitActing()) {
      clearSelection();
      mapManager.endTurn();
    }
  }

  if (const auto *mp = event.getIf<sf::Event::MouseButtonPressed>()) {
    if (mp->button == sf::Mouse::Button::Right) {
      clearSelection();
    } else if (mp->button == sf::Mouse::Button::Left) {
      sf::Vector2f worldPos = m_window.mapPixelToCoords({mp->position.x, mp->position.y}, m_gameView);
      int tileX = static_cast<int>(worldPos.x / tw);
      int tileY = static_cast<int>(worldPos.y / th);
      if (tileX >= 0 && tileX < static_cast<int>(mapManager.getMapWidth()) &&
          tileY >= 0 && tileY < static_cast<int>(mapManager.getMapHeight())) {
        handleMouseClicked({tileX, tileY});
      }
    }
  }
}

void SelectionController::handleMouseMoved(sf::Vector2i gridPos, sf::Vector2f localPos) {
  if (gridPos.x < 0 || mapManager.isAnyUnitActing() || !selectedUnit) {
    if (!selectedUnit) {
      previewPath.clear();
      hoveredEnemyUnit = nullptr;
      m_preferredApproachDir = {0, 0};
      m_cursorIconCell = -1;
    }
    return;
  }

  const sf::Vector2u tileSize = mapManager.getTileSize();
  sf::Vector2i unitGrid(
      static_cast<int>(std::round(selectedUnit->getPosition().x / static_cast<float>(tileSize.x))),
      static_cast<int>(std::round(selectedUnit->getPosition().y / static_cast<float>(tileSize.y))));

  auto unitAtTile = mapManager.getUnitAtTile(gridPos);
  
  if (unitAtTile && unitAtTile != selectedUnit &&
      unitAtTile->getTeam() != selectedUnit->getTeam() &&
      selectedUnit->canTarget(*unitAtTile)) {
    const int minRange = selectedUnit->getMinAttackRange();
    const int maxRange = selectedUnit->getMaxAttackRange();
    auto canAttackFrom = [&](sf::Vector2i from) {
      const int dx = std::abs(gridPos.x - from.x);
      const int dy = std::abs(gridPos.y - from.y);
      const int distance = (maxRange == 1) ? (dx + dy) : std::max(dx, dy);
      return distance >= minRange && distance <= maxRange;
    };

    bool canAttackThisAction = canAttackFrom(unitGrid);
    if (!canAttackThisAction) {
      for (const auto &tile : reachableTiles) {
        if (canAttackFrom(tile)) {
          canAttackThisAction = true;
          break;
        }
      }
    }

    if (!canAttackThisAction) {
      hoveredEnemyUnit = nullptr;
      previewPath.clear();
      m_preferredApproachDir = {0, 0};
      m_cursorIconCell = -1;
      return;
    }

    hoveredEnemyUnit = unitAtTile;
    m_cursorIconCell = 1;
    
    sf::Vector2i newDir = computeApproachDir(localPos, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y));
    if (newDir != m_preferredApproachDir) {
      m_preferredApproachDir = newDir;
      updateAttackPath(gridPos, unitGrid, m_preferredApproachDir);
    } else if (previewPath.empty()) {
        updateAttackPath(gridPos, unitGrid, m_preferredApproachDir);
    }
    return;
  }

  hoveredEnemyUnit = nullptr;

  bool isReachable = std::find(reachableTiles.begin(), reachableTiles.end(), gridPos) != reachableTiles.end();
  if (!isReachable) {
    previewPath.clear();
    m_cursorIconCell = -1;
    return;
  }
  previewPath = mapManager.findPath(unitGrid, gridPos, selectedUnit->getTeam(), selectedUnit->getMovementCategory());
  m_cursorIconCell = 0;
}

void SelectionController::handleMouseClicked(sf::Vector2i gridPos) {
  if (mapManager.isAnyUnitActing()) return;
  const sf::Vector2u tileSize = mapManager.getTileSize();

  if (hoveredEnemyUnit && selectedUnit) {
    sf::Vector2i attackerGrid(
        static_cast<int>(std::round(selectedUnit->getPosition().x / static_cast<float>(tileSize.x))),
        static_cast<int>(std::round(selectedUnit->getPosition().y / static_cast<float>(tileSize.y))));
    sf::Vector2i attackerEnd = previewPath.empty() ? attackerGrid : previewPath.back();

    int dx = gridPos.x - attackerEnd.x;
    int dy = gridPos.y - attackerEnd.y;

    int distanceToTarget = (selectedUnit->getMaxAttackRange() == 1)
                               ? (std::abs(dx) + std::abs(dy))
                               : std::max(std::abs(dx), std::abs(dy));

    if (distanceToTarget < selectedUnit->getMinAttackRange() ||
        distanceToTarget > selectedUnit->getMaxAttackRange()) {
      if (!previewPath.empty()) {
        mapManager.pushUndoState(attackerGrid);
        selectedUnit->move(previewPath);
        m_turnController.markActed(*selectedUnit);
      }
      clearSelection();
      return;
    }

    MoveDirection shootDir =
        (std::abs(dx) >= std::abs(dy))
            ? (dx >= 0 ? MoveDirection::Right : MoveDirection::Left)
            : (dy >= 0 ? MoveDirection::Down : MoveDirection::Up);

    if (!previewPath.empty() && selectedUnit->getMaxAttackRange() > 1) {
      mapManager.pushUndoState(attackerGrid);
      selectedUnit->move(previewPath);
      m_turnController.markActed(*selectedUnit);
      clearSelection();
      return;
    }

    mapManager.pushUndoState(attackerGrid);
    if (!previewPath.empty()) {
      selectedUnit->move(previewPath);
    }

    auto unitToMark = selectedUnit;
    auto targetRef = hoveredEnemyUnit;
    auto *self = this;
    unitToMark->onAttackFinished = [self, unitToMark, targetRef]() {
      self->mapManager.runWhenAllActionsFinished(
          [self, unitToMark, targetRef]() {
            if (targetRef->isDead()) {
              self->mapManager.clearUndoStack();
            }
            self->m_turnController.markActed(*unitToMark);
          });
    };

    selectedUnit->dealDamage(hoveredEnemyUnit, shootDir);
    clearSelection();
    return;
  }

  auto unitAtTile = mapManager.getUnitAtTile(gridPos);
  if (unitAtTile) {
    if (unitAtTile->getTeam() == Team::Enemy) return;
    if (!m_turnController.canAct(*unitAtTile)) return;
    if (!unitAtTile->getIsInteractable()) return;
    
    selectedUnit = unitAtTile;
    m_state = SelectionState::UnitSelected;
    sf::Vector2i unitGrid(
        static_cast<int>(std::round(unitAtTile->getPosition().x / static_cast<float>(tileSize.x))),
        static_cast<int>(std::round(unitAtTile->getPosition().y / static_cast<float>(tileSize.y))));
    reachableTiles = mapManager.getReachableTiles(
        unitGrid, unitAtTile->getMoveSpeed(), unitAtTile->getTeam(),
        unitAtTile->getMovementCategory());
    previewPath.clear();
    hoveredEnemyUnit = nullptr;
    return;
  }

  if (!selectedUnit) {
    auto building = mapManager.getBuildingAtTile(gridPos);
    if (building) {
      if (building->getTypeName() == "Factory" &&
          building->getTeam() == m_turnController.getCurrentTeam()) {
        if (!mapManager.getUnitAtTile(gridPos)) {
          openFactoryUI(building);
        }
      } else {
        building->onClicked();
      }
    }
    return;
  }

  bool isReachable = std::find(reachableTiles.begin(), reachableTiles.end(), gridPos) != reachableTiles.end();
  if (isReachable && !previewPath.empty()) {
    sf::Vector2i unitGrid2(
        static_cast<int>(std::round(selectedUnit->getPosition().x / static_cast<float>(tileSize.x))),
        static_cast<int>(std::round(selectedUnit->getPosition().y / static_cast<float>(tileSize.y))));
    mapManager.pushUndoState(unitGrid2);
    if (mapManager.onUnitMoveStart)
      mapManager.onUnitMoveStart(selectedUnit);
    selectedUnit->move(previewPath);
    m_turnController.markActed(*selectedUnit);
  }

  clearSelection();
}

void SelectionController::drawOverlays(sf::RenderTarget &target) {
  if (m_turnController.getCurrentTeam() == Team::Enemy)
    return;

  const sf::Vector2u tileSize = mapManager.getTileSize();
  const float tw = static_cast<float>(tileSize.x);
  const float th = static_cast<float>(tileSize.y);

  std::vector<sf::Vector2i> enemyOverlayTiles;
  if (selectedUnit) {
    const sf::Vector2i unitGrid(
        static_cast<int>(std::round(selectedUnit->getPosition().x / tw)),
        static_cast<int>(std::round(selectedUnit->getPosition().y / th)));
    const int minRange = selectedUnit->getMinAttackRange();
    const int maxRange = selectedUnit->getMaxAttackRange();
    const int mapW = static_cast<int>(mapManager.getMapWidth());
    const int mapH = static_cast<int>(mapManager.getMapHeight());

    for (const auto &unit : mapManager.getUnits()) {
      if (unit->getTeam() == selectedUnit->getTeam() || unit->isDead())
        continue;
      if (!selectedUnit->canTarget(*unit))
        continue;

      const sf::Vector2i enemyGrid(
          static_cast<int>(std::round(unit->getPosition().x / tw)),
          static_cast<int>(std::round(unit->getPosition().y / th)));

      auto inRange = [&](sf::Vector2i from) {
        int dx = std::abs(enemyGrid.x - from.x);
        int dy = std::abs(enemyGrid.y - from.y);
        int d = (maxRange == 1) ? (dx + dy) : std::max(dx, dy);
        return d >= minRange && d <= maxRange;
      };

      bool reachable = inRange(unitGrid);
      if (!reachable) {
        for (const auto &tile : reachableTiles) {
          if (inRange(tile)) {
            reachable = true;
            break;
          }
        }
      }
      if (!reachable)
        continue;

      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          sf::Vector2i boxTile(enemyGrid.x + dx, enemyGrid.y + dy);
          if (boxTile.x >= 0 && boxTile.y >= 0 && boxTile.x < mapW &&
              boxTile.y < mapH) {
            if (std::find(enemyOverlayTiles.begin(), enemyOverlayTiles.end(),
                          boxTile) == enemyOverlayTiles.end())
              enemyOverlayTiles.push_back(boxTile);
          }
        }
      }
    }
  }

  sf::Sprite walkSprite(m_walkOverlayTexture);
  for (const auto &pos : reachableTiles) {
    if (std::find(enemyOverlayTiles.begin(), enemyOverlayTiles.end(), pos) !=
        enemyOverlayTiles.end())
      continue;
    walkSprite.setPosition(sf::Vector2f(pos.x * tw, pos.y * th));
    target.draw(walkSprite);
  }

  if (!enemyOverlayTiles.empty()) {
    sf::Sprite enemySprite(m_enemyOverlayTexture);
    for (const auto &pos : enemyOverlayTiles) {
      enemySprite.setPosition(sf::Vector2f(pos.x * tw, pos.y * th));
      target.draw(enemySprite);
    }
  }

  if (!previewPath.empty() && selectedUnit) {
    sf::Vector2i unitGrid(
        static_cast<int>(std::round(selectedUnit->getPosition().x / tw)),
        static_cast<int>(std::round(selectedUnit->getPosition().y / th)));

    auto dirAngle = [](sf::Vector2i from, sf::Vector2i to) -> float {
      if (to.x > from.x) return 0.f;
      if (to.x < from.x) return 180.f;
      if (to.y > from.y) return 90.f;
      return 270.f;
    };

    auto cornerAngle = [](sf::Vector2i inDir, sf::Vector2i outDir) -> float {
      if ((inDir.x > 0 && outDir.y > 0) || (inDir.y < 0 && outDir.x < 0)) return 0.f;
      if ((inDir.y > 0 && outDir.x < 0) || (inDir.x > 0 && outDir.y < 0)) return 90.f;
      if ((inDir.x < 0 && outDir.y < 0) || (inDir.y > 0 && outDir.x > 0)) return 180.f;
      return 270.f;
    };

    sf::Sprite arrowSprite(m_moveArrowTexture);
    arrowSprite.setOrigin({16.f, 16.f});

    arrowSprite.setTextureRect(sf::IntRect({0, 0}, {32, 32}));
    arrowSprite.setRotation(sf::degrees(dirAngle(unitGrid, previewPath[0])));
    arrowSprite.setPosition({unitGrid.x * tw + tw * 0.5f, unitGrid.y * th + th * 0.5f});
    target.draw(arrowSprite);

    for (int i = 0; i < static_cast<int>(previewPath.size()); ++i) {
      const sf::Vector2i &cur = previewPath[i];
      sf::Vector2i prev = (i == 0) ? unitGrid : previewPath[i - 1];
      bool isLast = (i == static_cast<int>(previewPath.size()) - 1);

      sf::Vector2i inDir = {cur.x - prev.x, cur.y - prev.y};
      float angle;
      int cellX;

      if (isLast) {
        cellX = 64;
        angle = dirAngle(prev, cur);
      } else {
        const sf::Vector2i &next = previewPath[i + 1];
        sf::Vector2i outDir = {next.x - cur.x, next.y - cur.y};
        if (inDir.x != outDir.x || inDir.y != outDir.y) {
          cellX = 32;
          angle = cornerAngle(inDir, outDir);
        } else {
          cellX = 0;
          angle = dirAngle(prev, cur);
        }
      }

      arrowSprite.setTextureRect(sf::IntRect({cellX, 0}, {32, 32}));
      arrowSprite.setRotation(sf::degrees(angle));
      arrowSprite.setPosition({cur.x * tw + tw * 0.5f, cur.y * th + th * 0.5f});
      target.draw(arrowSprite);
    }

    const sf::Vector2i &end = previewPath.back();
    sf::IntRect rect = selectedUnit->getCurrentRect();
    sf::Sprite ghost(selectedUnit->getCurrentTexture());
    ghost.setTextureRect(rect);
    ghost.setColor(sf::Color(255, 255, 255, 150));
    if (selectedUnit->shouldFlipX()) {
      ghost.setScale({-1.f, 1.f});
      ghost.setPosition({end.x * tw + static_cast<float>(rect.size.x), end.y * th});
    } else {
      ghost.setPosition({end.x * tw, end.y * th});
    }
    target.draw(ghost);
  }

  if (hoveredEnemyUnit) {
    sf::RectangleShape overlay(sf::Vector2f(tw, th));
    sf::Vector2i enemyGrid(
        static_cast<int>(std::round(hoveredEnemyUnit->getPosition().x / tw)),
        static_cast<int>(std::round(hoveredEnemyUnit->getPosition().y / th)));
    overlay.setFillColor(sf::Color(255, 50, 50, 160));
    overlay.setPosition(sf::Vector2f(enemyGrid.x * tw, enemyGrid.y * th));
    target.draw(overlay);
  }

  for (const auto &building : mapManager.getBuildings()) {
    if (building->getTypeName() == "Factory" &&
        building->getTeam() == m_turnController.getCurrentTeam()) {
      sf::Vector2i bGrid(
          static_cast<int>(std::round(building->getPosition().x / tw)),
          static_cast<int>(std::round(building->getPosition().y / th)));
      if (!mapManager.getUnitAtTile(bGrid)) {
        sf::Sprite friendlyOverlay(m_friendlyOverlayTexture);
        friendlyOverlay.setPosition(sf::Vector2f(bGrid.x * tw, bGrid.y * th));
        target.draw(friendlyOverlay);
      }
    }
  }
}

void SelectionController::drawCursorIcon(sf::RenderTarget &target) {
  if (m_cursorIconCell < 0) return;

  sf::View savedView = target.getView();
  target.setView(target.getDefaultView());

  sf::Sprite icon(m_iconsTexture);
  icon.setTextureRect(sf::IntRect({m_cursorIconCell * 32, 0}, {32, 32}));
  icon.setScale({1.2f, 1.2f});
  icon.setPosition({m_screenCursorPos.x + 15.f, m_screenCursorPos.y + 15.f});
  target.draw(icon);

  target.setView(savedView);
}

void SelectionController::clearSelection() {
  selectedUnit = nullptr;
  m_state = SelectionState::Idle;
  reachableTiles.clear();
  hoveredEnemyUnit = nullptr;
  m_cursorIconCell = -1;
  previewPath.clear();
}

void SelectionController::selectUnit(std::shared_ptr<Unit> unit) {
  if (!unit || unit->isDead() || !m_turnController.canAct(*unit)) return;
  selectedUnit = unit;
  m_state = SelectionState::UnitSelected;
  const sf::Vector2u tileSize = mapManager.getTileSize();
  sf::Vector2i unitGrid(
      static_cast<int>(std::round(unit->getPosition().x / static_cast<float>(tileSize.x))),
      static_cast<int>(std::round(unit->getPosition().y / static_cast<float>(tileSize.y))));
  reachableTiles = mapManager.getReachableTiles(unitGrid, unit->getMoveSpeed(),
                                                unit->getTeam(),
                                                unit->getMovementCategory());
  previewPath.clear();
  hoveredEnemyUnit = nullptr;
  m_cursorIconCell = -1;
}

void SelectionController::syncCameraView(sf::View gameView) {
  m_gameView = gameView;
}

void SelectionController::update(float /*dt*/) {
}

void SelectionController::updateAttackPath(sf::Vector2i enemyGrid, sf::Vector2i unitGrid, sf::Vector2i preferredDir) {
  previewPath.clear();

  int minRange = selectedUnit->getMinAttackRange();
  int maxRange = selectedUnit->getMaxAttackRange();

  auto inRange = [&](sf::Vector2i tile) {
    int dx = std::abs(enemyGrid.x - tile.x);
    int dy = std::abs(enemyGrid.y - tile.y);
    int d = (maxRange == 1) ? (dx + dy) : std::max(dx, dy);
    return d >= minRange && d <= maxRange;
  };

  if (inRange(unitGrid)) return;

  if (preferredDir != sf::Vector2i{0, 0}) {
    sf::Vector2i preferred = enemyGrid + preferredDir;
    if (preferred != unitGrid && inRange(preferred) &&
        std::find(reachableTiles.begin(), reachableTiles.end(), preferred) != reachableTiles.end()) {
      auto path = mapManager.findPath(unitGrid, preferred, selectedUnit->getTeam(), selectedUnit->getMovementCategory());
      if (!path.empty()) {
        previewPath = path;
        return;
      }
    }
  }

  std::vector<sf::Vector2i> bestPath;
  for (const auto &tile : reachableTiles) {
    if (!inRange(tile)) continue;
    auto path = mapManager.findPath(unitGrid, tile, selectedUnit->getTeam(), selectedUnit->getMovementCategory());
    if (!path.empty() && (bestPath.empty() || path.size() < bestPath.size())) {
      bestPath = path;
    }
  }
  previewPath = bestPath;
}

void SelectionController::openFactoryUI(std::shared_ptr<Building> factory) {
  if (m_onOpenFactory) m_onOpenFactory(factory);
}
