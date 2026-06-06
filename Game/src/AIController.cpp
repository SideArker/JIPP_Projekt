#include "AIController.hpp"
#include "Building.hpp"
#include "CameraController.hpp"
#include "UnitRegistry.hpp"
#include <algorithm>
#include <climits>
#include <cmath>

static sf::Vector2i gridOf(const Unit &u, sf::Vector2u ts) {
  return {static_cast<int>(
              std::round(u.getPosition().x / static_cast<float>(ts.x))),
          static_cast<int>(
              std::round(u.getPosition().y / static_cast<float>(ts.y)))};
}

static sf::Vector2i buildingGridOf(const Building &b, sf::Vector2u ts) {
  return {static_cast<int>(
              std::round(b.getPosition().x / static_cast<float>(ts.x))),
          static_cast<int>(
              std::round(b.getPosition().y / static_cast<float>(ts.y)))};
}

static MoveDirection directionBetween(sf::Vector2i from, sf::Vector2i to) {
  int dx = to.x - from.x, dy = to.y - from.y;
  if (std::abs(dx) >= std::abs(dy))
    return dx >= 0 ? MoveDirection::Right : MoveDirection::Left;
  return dy >= 0 ? MoveDirection::Down : MoveDirection::Up;
}

bool AIController::moveUnitToward(Unit &unit, sf::Vector2i from,
                                  sf::Vector2i target,
                                  const std::vector<sf::Vector2i> &reachable,
                                  MapManager &mapManager, TurnController &tc) {
  if (from == target) {
    tc.markActed(unit);
    return true;
  }

  auto fullPath = mapManager.findPath(from, target, unit.getTeam(),
                                      unit.getMovementCategory());
  if (!fullPath.empty()) {
    if (std::find(reachable.begin(), reachable.end(), target) !=
        reachable.end()) {
      unit.move(fullPath);
      tc.markActed(unit);
      return true;
    } else {
      // Find furthest reachable tile along the path
      std::vector<sf::Vector2i> bestPath;
      for (int i = static_cast<int>(fullPath.size()) - 1; i >= 0; --i) {
        if (std::find(reachable.begin(), reachable.end(), fullPath[i]) !=
            reachable.end()) {
          bestPath = std::vector<sf::Vector2i>(fullPath.begin(),
                                               fullPath.begin() + i + 1);
          break;
        }
      }
      if (!bestPath.empty()) {
        unit.move(bestPath);
        tc.markActed(unit);
        return true;
      }
    }
  }

  sf::Vector2i best = from;
  int bestDist = INT_MAX;
  for (const auto &tile : reachable) {
    int dist = std::abs(tile.x - target.x) + std::abs(tile.y - target.y);
    if (dist < bestDist) {
      bestDist = dist;
      best = tile;
    }
  }

  if (best != from) {
    auto path = mapManager.findPath(from, best, unit.getTeam(),
                                    unit.getMovementCategory());
    if (!path.empty())
      unit.move(path);
  }
  tc.markActed(unit);
  return true;
}

void AIController::reset() {
  m_queue.clear();
  m_productionQueue.clear();
  m_productionEvaluated = false;
  m_waitTimer = 0.f;
  m_started = false;
}

void AIController::buildQueue(MapManager &mapManager) {
  m_queue.clear();
  for (const auto &unit : mapManager.getUnits()) {
    if (!unit->isDead() && unit->getTeam() == Team::Enemy && !unit->hasActed())
      m_queue.push_back(unit);
  }
}

void AIController::update(float dt, MapManager &mapManager, TurnController &tc,
                          CameraController &camera) {
  if (!m_started) {
    buildQueue(mapManager);
    m_started = true;
  }

  if (m_waitTimer > 0.f) {
    m_waitTimer -= dt;
    return;
  }
  if (mapManager.isAnyUnitActing())
    return;
  if (m_queue.empty()) {
    if (!m_productionEvaluated) {
      evaluateProduction(mapManager);
      m_productionEvaluated = true;
      m_waitTimer = 1.0f;
      return;
    }
    if (!m_productionQueue.empty()) {
      processProduction(mapManager, tc, camera);
      m_waitTimer = 1.0f;
      return;
    }
    camera.release();
    return;
  }

  processNextUnit(mapManager, tc, camera);
  m_waitTimer = ACTION_DELAY;
}

void AIController::processNextUnit(MapManager &mapManager, TurnController &tc,
                                   CameraController &camera) {
  camera.release();

  while (!m_queue.empty()) {
    auto unit = m_queue.front();
    m_queue.erase(m_queue.begin());

    if (unit->isDead() || unit->hasActed())
      continue;

    camera.trackUnit(unit);

    bool acted = false;

    // Capture logic
    if (unit->hasFlag(UnitFlag::Capture)) {
      sf::Vector2u ts = mapManager.getTileSize();
      sf::Vector2i uGrid = gridOf(*unit, ts);
      auto b = mapManager.getBuildingAtTile(uGrid);
      if (b && b->getTeam() != Team::Enemy) {
        tc.markActed(*unit);
        acted = true;
      }
    }

    bool openingNeutralFocus = tc.getTurnNumber() <= 2;

    if (!acted && openingNeutralFocus)
      acted = tryConquerNeutral(*unit, mapManager, tc);

    if (!acted)
      acted = tryAttackUnit(*unit, mapManager, tc);
    if (!acted)
      acted = tryCapturePlayerBuilding(*unit, mapManager, tc);
    if (!acted)
      acted = tryBlockProductionBuilding(*unit, mapManager, tc);
    if (!acted)
      acted = tryConquerNeutral(*unit, mapManager, tc);

    if (!acted)
      tryMoveTowardPlayer(*unit, mapManager, tc);

    return;
  }
}

bool AIController::tryConquerNeutral(Unit &unit, MapManager &mapManager,
                                     TurnController &tc) {
  if (!unit.hasFlag(UnitFlag::Capture))
    return false;

  sf::Vector2u ts = mapManager.getTileSize();
  sf::Vector2i uGrid = gridOf(unit, ts);
  auto rawReachable = mapManager.getReachableTiles(
      uGrid, unit.getMoveSpeed(), unit.getTeam(), unit.getMovementCategory());
  std::vector<sf::Vector2i> reachable;
  for (const auto &t : rawReachable) {
    if (!isTileBlocking(t, unit, mapManager))
      reachable.push_back(t);
  }
  if (reachable.empty())
    reachable = rawReachable;

  const std::shared_ptr<Building> *best = nullptr;
  size_t bestDist = 99999;

  for (const auto &b : mapManager.getBuildings()) {
    if (b->getTeam() != Team::Neutral)
      continue;
    sf::Vector2i bGrid = buildingGridOf(*b, ts);
    auto occupant = mapManager.getUnitAtTile(bGrid);
    if (occupant && occupant->getTeam() == Team::Enemy)
      continue;

    auto path = mapManager.findPath(uGrid, bGrid, unit.getTeam(),
                                    unit.getMovementCategory());
    if (path.empty())
      continue;

    if (path.size() < bestDist) {
      bestDist = path.size();
      best = &b;
    }
  }

  if (!best)
    return false;

  return moveUnitToward(unit, uGrid, buildingGridOf(**best, ts), reachable,
                        mapManager, tc);
}

bool AIController::tryCapturePlayerBuilding(Unit &unit, MapManager &mapManager,
                                            TurnController &tc) {
  if (!unit.hasFlag(UnitFlag::Capture))
    return false;

  sf::Vector2u ts = mapManager.getTileSize();
  sf::Vector2i uGrid = gridOf(unit, ts);
  auto rawReachable = mapManager.getReachableTiles(
      uGrid, unit.getMoveSpeed(), unit.getTeam(), unit.getMovementCategory());
  std::vector<sf::Vector2i> reachable;
  for (const auto &t : rawReachable) {
    if (!isTileBlocking(t, unit, mapManager))
      reachable.push_back(t);
  }
  if (reachable.empty())
    reachable = rawReachable;

  const std::shared_ptr<Building> *best = nullptr;
  size_t bestDist = 99999;

  for (const auto &b : mapManager.getBuildings()) {
    if (b->getTeam() != Team::Ally)
      continue;
    sf::Vector2i bGrid = buildingGridOf(*b, ts);
    auto occupant = mapManager.getUnitAtTile(bGrid);
    if (occupant && occupant->getTeam() == Team::Enemy)
      continue;

    auto path = mapManager.findPath(uGrid, bGrid, unit.getTeam(),
                                    unit.getMovementCategory());
    if (path.empty())
      continue;

    if (path.size() < bestDist) {
      bestDist = path.size();
      best = &b;
    }
  }

  if (!best)
    return false;

  return moveUnitToward(unit, uGrid, buildingGridOf(**best, ts), reachable,
                        mapManager, tc);
}

bool AIController::tryBlockProductionBuilding(Unit &unit,
                                              MapManager &mapManager,
                                              TurnController &tc) {
  // Only triggered when no alive enemy capture unit exists
  bool hasConquerUnit = false;
  for (const auto &u : mapManager.getUnits()) {
    if (!u->isDead() && u->getTeam() == Team::Enemy &&
        u->hasFlag(UnitFlag::Capture)) {
      hasConquerUnit = true;
      break;
    }
  }
  if (hasConquerUnit)
    return false;

  sf::Vector2u ts = mapManager.getTileSize();
  sf::Vector2i uGrid = gridOf(unit, ts);
  auto rawReachable = mapManager.getReachableTiles(
      uGrid, unit.getMoveSpeed(), unit.getTeam(), unit.getMovementCategory());
  std::vector<sf::Vector2i> reachable;
  for (const auto &t : rawReachable) {
    if (!isTileBlocking(t, unit, mapManager))
      reachable.push_back(t);
  }
  if (reachable.empty())
    reachable = rawReachable;

  const std::shared_ptr<Building> *best = nullptr;
  size_t bestDist = 99999;

  for (const auto &b : mapManager.getBuildings()) {
    if (b->getTeam() != Team::Ally)
      continue;
    const std::string &type = b->getTypeName();
    if (type != "Factory" && type != "HQ")
      continue;
    sf::Vector2i bGrid = buildingGridOf(*b, ts);
    auto occupant = mapManager.getUnitAtTile(bGrid);
    if (occupant && occupant->getTeam() == Team::Enemy)
      continue;

    auto path = mapManager.findPath(uGrid, bGrid, unit.getTeam(),
                                    unit.getMovementCategory());
    if (path.empty())
      continue;

    if (path.size() < bestDist) {
      bestDist = path.size();
      best = &b;
    }
  }

  if (!best)
    return false;

  return moveUnitToward(unit, uGrid, buildingGridOf(**best, ts), reachable,
                        mapManager, tc);
}

bool AIController::tryAttackUnit(Unit &unit, MapManager &mapManager,
                                 TurnController &tc) {
  sf::Vector2u ts = mapManager.getTileSize();
  sf::Vector2i uGrid = gridOf(unit, ts);
  int maxR = unit.getMaxAttackRange();
  int minR = unit.getMinAttackRange();

  auto inRange = [&](sf::Vector2i from, sf::Vector2i to) {
    int dx = std::abs(to.x - from.x), dy = std::abs(to.y - from.y);
    int d = (maxR == 1) ? (dx + dy) : std::max(dx, dy);
    return d >= minR && d <= maxR;
  };

  auto rawReachable = mapManager.getReachableTiles(
      uGrid, unit.getMoveSpeed(), unit.getTeam(), unit.getMovementCategory());
  std::vector<sf::Vector2i> reachable;
  for (const auto &t : rawReachable) {
    if (!isTileBlocking(t, unit, mapManager))
      reachable.push_back(t);
  }
  if (reachable.empty())
    reachable = rawReachable;

  struct Candidate {
    std::shared_ptr<Unit> target;
    std::vector<sf::Vector2i> movePath;
    sf::Vector2i attackFrom;
  };

  std::vector<Candidate> candidates;

  for (const auto &ally : mapManager.getUnits()) {
    if (ally->isDead() || ally->getTeam() != Team::Ally)
      continue;
    if (!unit.canTarget(*ally))
      continue;
    sf::Vector2i aGrid = gridOf(*ally, ts);

    if (inRange(uGrid, aGrid) && !isTileBlocking(uGrid, unit, mapManager)) {
      candidates.push_back({ally, {}, uGrid});
      continue;
    }

    // Find closest reachable tile in attack range
    std::vector<sf::Vector2i> bestPath;
    sf::Vector2i bestTile = uGrid;
    for (const auto &tile : reachable) {
      if (!inRange(tile, aGrid))
        continue;
      auto path = mapManager.findPath(uGrid, tile, unit.getTeam(),
                                      unit.getMovementCategory());
      if (!path.empty() &&
          (bestPath.empty() || path.size() < bestPath.size())) {
        bestPath = path;
        bestTile = tile;
      }
    }
    if (!bestPath.empty())
      candidates.push_back({ally, bestPath, bestTile});
  }

  if (candidates.empty())
    return false;

  // Prefer target with lowest health
  auto &best =
      *std::min_element(candidates.begin(), candidates.end(),
                        [](const Candidate &a, const Candidate &b) {
                          bool aInteractable = a.target->getIsInteractable();
                          bool bInteractable = b.target->getIsInteractable();
                          if (aInteractable != bInteractable)
                            return aInteractable > bInteractable;
                          return a.target->getHealth() < b.target->getHealth();
                        });

  if (!best.movePath.empty()) {
    if (maxR > 1) {
      // Ranged: just move this turn, can't attack after moving
      unit.move(best.movePath);
      tc.markActed(unit);
      return true;
    }
    unit.move(best.movePath);
  }

  sf::Vector2i tGrid = gridOf(*best.target, ts);
  unit.dealDamage(best.target, directionBetween(best.attackFrom, tGrid));
  tc.markActed(unit);
  return true;
}

bool AIController::tryMoveTowardPlayer(Unit &unit, MapManager &mapManager,
                                       TurnController &tc) {
  sf::Vector2u ts = mapManager.getTileSize();
  sf::Vector2i uGrid = gridOf(unit, ts);
  auto rawReachable = mapManager.getReachableTiles(
      uGrid, unit.getMoveSpeed(), unit.getTeam(), unit.getMovementCategory());
  std::vector<sf::Vector2i> reachable;
  for (const auto &t : rawReachable) {
    if (!isTileBlocking(t, unit, mapManager))
      reachable.push_back(t);
  }
  if (reachable.empty())
    reachable = rawReachable;

  // Find nearest ally unit by path distance
  const std::shared_ptr<Unit> *nearest = nullptr;
  size_t bestDist = 99999;
  for (const auto &ally : mapManager.getUnits()) {
    if (ally->isDead() || ally->getTeam() != Team::Ally)
      continue;
    if (!unit.canTarget(*ally))
      continue;
    sf::Vector2i aGrid = gridOf(*ally, ts);

    auto path = mapManager.findPath(uGrid, aGrid, unit.getTeam(),
                                    unit.getMovementCategory());
    if (path.empty())
      continue; // No path available

    size_t effectiveDist = path.size();
    if (!ally->getIsInteractable()) {
      effectiveDist += 10000;
    }

    if (effectiveDist < bestDist) {
      bestDist = effectiveDist;
      nearest = &ally;
    }
  }

  if (!nearest) {
    tc.markActed(unit);
    return true;
  }

  sf::Vector2i target = gridOf(**nearest, ts);
  return moveUnitToward(unit, uGrid, target, reachable, mapManager, tc);
}

void AIController::evaluateProduction(MapManager &mapManager) {
  m_productionQueue.clear();

  int currentMoney = 0;
  auto moneyIt = mapManager.getTeams().find(Team::Enemy);
  if (moneyIt != mapManager.getTeams().end()) {
    currentMoney = moneyIt->second.money;
  }
  if (currentMoney <= 0)
    return;

  bool hasAirport = false, hasPort = false, hasVehicleBase = false;
  std::vector<std::shared_ptr<Building>> factories;

  for (const auto &b : mapManager.getBuildings()) {
    if (b->getTeam() == Team::Enemy) {
      const std::string &type = b->getTypeName();
      if (type == "Factory")
        factories.push_back(b);
      else if (type == "Airport")
        hasAirport = true;
      else if (type == "VehicleBase")
        hasVehicleBase = true;
      else if (type == "Port")
        hasPort = true;
    }
  }

  if (factories.empty())
    return;

  std::vector<std::shared_ptr<Building>> unoccupiedFactories;
  for (auto &f : factories) {
    sf::Vector2i gridPos = buildingGridOf(*f, mapManager.getTileSize());
    if (!mapManager.getUnitAtTile(gridPos)) {
      unoccupiedFactories.push_back(f);
    }
  }

  if (unoccupiedFactories.empty())
    return;

  auto unitNames = UnitRegistry::getRegisteredUnitNames();
  std::vector<std::pair<std::string, const UnitData *>> availableUnits;
  for (const auto &uName : unitNames) {
    const UnitData *d = UnitRegistry::getData(uName);
    if (!d)
      continue;
    bool allowed = false;
    switch (d->movementCategory) {
    case MovementCategory::Infantry:
      allowed = true;
      break;
    case MovementCategory::Ground:
      allowed = hasVehicleBase;
      break;
    case MovementCategory::Flying:
      allowed = hasAirport;
      break;
    case MovementCategory::Naval:
      allowed = hasPort;
      break;
    case MovementCategory::None:
      allowed = false;
      break;
    }
    if (allowed && d->cost > 0) {
      availableUnits.push_back({uName, d});
    }
  }

  int captureUnits = 0;
  for (const auto &u : mapManager.getUnits()) {
    if (!u->isDead() && u->getTeam() == Team::Enemy &&
        u->hasFlag(UnitFlag::Capture)) {
      captureUnits++;
    }
  }

  int capturableBuildings = 0;
  for (const auto &b : mapManager.getBuildings()) {
    if (b->getTeam() != Team::Enemy)
      capturableBuildings++;
  }

  bool needCapture = (capturableBuildings > 0 && captureUnits < 2);

  for (auto &factory : unoccupiedFactories) {
    if (currentMoney <= 0)
      break;

    const UnitData *bestChoice = nullptr;
    std::string bestName = "";

    if (needCapture) {
      for (const auto &pair : availableUnits) {
        if (pair.first == "Soldier" && pair.second->cost <= currentMoney) {
          bestChoice = pair.second;
          bestName = pair.first;
          break;
        }
      }
      if (bestChoice)
        needCapture = false;
    }

    if (!bestChoice) {
      for (const auto &pair : availableUnits) {
        if (pair.second->cost <= currentMoney) {
          if (!bestChoice || pair.second->cost > bestChoice->cost) {
            bestChoice = pair.second;
            bestName = pair.first;
          }
        }
      }
    }

    if (bestChoice) {
      currentMoney -= bestChoice->cost;
      m_productionQueue.push_back({factory, bestName, bestChoice->cost});
    }
  }
}

void AIController::processProduction(MapManager &mapManager, TurnController &tc,
                                     CameraController &camera) {
  if (m_productionQueue.empty())
    return;

  auto task = m_productionQueue.front();
  m_productionQueue.erase(m_productionQueue.begin());

  auto newUnit = UnitRegistry::create(task.unitName, Team::Enemy);
  if (newUnit) {
    newUnit->setActed(true);
    newUnit->setFadeIn(0.3f);
    mapManager.deductTeamMoney(Team::Enemy, task.cost);

    sf::Vector2u ts = mapManager.getTileSize();
    sf::Vector2i gridPos = buildingGridOf(*task.factory, ts);
    mapManager.spawnUnit(newUnit, gridPos.x, gridPos.y);

    camera.trackUnit(newUnit);
  }
}

bool AIController::isTileBlocking(sf::Vector2i tile, const Unit &unit,
                                  MapManager &mapManager) const {
  auto b = mapManager.getBuildingAtTile(tile);
  if (!b)
    return false;

  if (b->getTeam() == Team::Enemy && b->getTypeName() == "Factory") {
    return true;
  }

  if (b->getTeam() != Team::Enemy && !unit.hasFlag(UnitFlag::Capture)) {
    sf::Vector2u ts = mapManager.getTileSize();
    for (const auto &ally : mapManager.getUnits()) {
      if (!ally->isDead() && ally->getTeam() == Team::Enemy &&
          ally->hasFlag(UnitFlag::Capture)) {
        sf::Vector2i aGrid = gridOf(*ally, ts);
        int dist = std::abs(tile.x - aGrid.x) + std::abs(tile.y - aGrid.y);
        if (dist <= 5)
          return true;
      }
    }
  }
  return false;
}
