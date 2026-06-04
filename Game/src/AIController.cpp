#include "AIController.hpp"
#include "CameraController.hpp"
#include "Building.hpp"
#include <algorithm>
#include <climits>
#include <cmath>

static sf::Vector2i gridOf(const Unit& u, sf::Vector2u ts) {
    return {
        static_cast<int>(std::round(u.getPosition().x / static_cast<float>(ts.x))),
        static_cast<int>(std::round(u.getPosition().y / static_cast<float>(ts.y)))
    };
}

static sf::Vector2i buildingGridOf(const Building& b, sf::Vector2u ts) {
    return {
        static_cast<int>(std::round(b.getPosition().x / static_cast<float>(ts.x))),
        static_cast<int>(std::round(b.getPosition().y / static_cast<float>(ts.y)))
    };
}

static MoveDirection directionBetween(sf::Vector2i from, sf::Vector2i to) {
    int dx = to.x - from.x, dy = to.y - from.y;
    if (std::abs(dx) >= std::abs(dy))
        return dx >= 0 ? MoveDirection::Right : MoveDirection::Left;
    return dy >= 0 ? MoveDirection::Down : MoveDirection::Up;
}

// Moves unit toward target within reachable tiles. Always marks acted and returns true.
bool AIController::moveUnitToward(Unit& unit, sf::Vector2i from, sf::Vector2i target,
                                  const std::vector<sf::Vector2i>& reachable,
                                  MapManager& mapManager, TurnController& tc) {
    if (from == target) {
        tc.markActed(unit);
        return true;
    }

    auto fullPath = mapManager.findPath(from, target, unit.getTeam(), unit.getMovementCategory());
    if (!fullPath.empty()) {
        if (std::find(reachable.begin(), reachable.end(), target) != reachable.end()) {
            unit.move(fullPath);
            tc.markActed(unit);
            return true;
        } else {
            // Find furthest reachable tile along the path
            std::vector<sf::Vector2i> bestPath;
            for (int i = static_cast<int>(fullPath.size()) - 1; i >= 0; --i) {
                if (std::find(reachable.begin(), reachable.end(), fullPath[i]) != reachable.end()) {
                    bestPath = std::vector<sf::Vector2i>(fullPath.begin(), fullPath.begin() + i + 1);
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

    // Fallback if no valid path exists at all
    sf::Vector2i best = from;
    int bestDist = INT_MAX;
    for (const auto& tile : reachable) {
        int dist = std::abs(tile.x - target.x) + std::abs(tile.y - target.y);
        if (dist < bestDist) { bestDist = dist; best = tile; }
    }

    if (best != from) {
        auto path = mapManager.findPath(from, best, unit.getTeam(), unit.getMovementCategory());
        if (!path.empty()) unit.move(path);
    }
    tc.markActed(unit);
    return true;
}

void AIController::reset() {
    m_queue.clear();
    m_waitTimer = 0.f;
    m_started   = false;
}

void AIController::buildQueue(MapManager& mapManager) {
    m_queue.clear();
    for (const auto& unit : mapManager.getUnits()) {
        if (!unit->isDead() && unit->getTeam() == Team::Enemy && !unit->hasActed())
            m_queue.push_back(unit);
    }
}

void AIController::update(float dt, MapManager& mapManager, TurnController& tc, CameraController& camera) {
    if (!m_started) {
        buildQueue(mapManager);
        m_started = true;
    }

    if (m_waitTimer > 0.f) { m_waitTimer -= dt; return; }
    if (mapManager.isAnyUnitActing()) return;
    if (m_queue.empty()) { camera.release(); return; }

    processNextUnit(mapManager, tc, camera);
    m_waitTimer = ACTION_DELAY;
}

void AIController::processNextUnit(MapManager& mapManager, TurnController& tc, CameraController& camera) {
    camera.release();

    while (!m_queue.empty()) {
        auto unit = m_queue.front();
        m_queue.erase(m_queue.begin());

        if (unit->isDead() || unit->hasActed()) continue;

        camera.trackUnit(unit);

        bool openingNeutralFocus = tc.getTurnNumber() <= 2;

        bool acted = false;
        if (openingNeutralFocus)
            acted = tryConquerNeutral(*unit, mapManager, tc);

        if (!acted) acted = tryAttackUnit(*unit, mapManager, tc);
        if (!acted) acted = tryCapturePlayerBuilding(*unit, mapManager, tc);
        if (!acted) acted = tryBlockProductionBuilding(*unit, mapManager, tc);
        if (!acted) acted = tryConquerNeutral(*unit, mapManager, tc);

        if (!acted)
            tryMoveTowardPlayer(*unit, mapManager, tc);

        return;
    }
}

bool AIController::tryConquerNeutral(Unit& unit, MapManager& mapManager, TurnController& tc) {
    if (!unit.hasFlag(UnitFlag::Capture)) return false;

    sf::Vector2u ts   = mapManager.getTileSize();
    sf::Vector2i uGrid = gridOf(unit, ts);
    auto reachable = mapManager.getReachableTiles(uGrid, unit.getMoveSpeed(), unit.getTeam(), unit.getMovementCategory());

    const std::shared_ptr<Building>* best = nullptr;
    size_t bestDist = 99999;

    for (const auto& b : mapManager.getBuildings()) {
        if (b->getTeam() != Team::Neutral) continue;
        sf::Vector2i bGrid = buildingGridOf(*b, ts);
        auto occupant = mapManager.getUnitAtTile(bGrid);
        if (occupant && occupant->getTeam() == Team::Enemy) continue;
        
        auto path = mapManager.findPath(uGrid, bGrid, unit.getTeam(), unit.getMovementCategory());
        if (path.empty()) continue;
        
        if (path.size() < bestDist) { bestDist = path.size(); best = &b; }
    }

    if (!best) return false;

    return moveUnitToward(unit, uGrid, buildingGridOf(**best, ts), reachable, mapManager, tc);
}

bool AIController::tryCapturePlayerBuilding(Unit& unit, MapManager& mapManager, TurnController& tc) {
    if (!unit.hasFlag(UnitFlag::Capture)) return false;

    sf::Vector2u ts    = mapManager.getTileSize();
    sf::Vector2i uGrid = gridOf(unit, ts);
    auto reachable = mapManager.getReachableTiles(uGrid, unit.getMoveSpeed(), unit.getTeam(), unit.getMovementCategory());

    const std::shared_ptr<Building>* best = nullptr;
    size_t bestDist = 99999;

    for (const auto& b : mapManager.getBuildings()) {
        if (b->getTeam() != Team::Ally) continue;
        sf::Vector2i bGrid = buildingGridOf(*b, ts);
        auto occupant = mapManager.getUnitAtTile(bGrid);
        if (occupant && occupant->getTeam() == Team::Enemy) continue;
        
        auto path = mapManager.findPath(uGrid, bGrid, unit.getTeam(), unit.getMovementCategory());
        if (path.empty()) continue;
        
        if (path.size() < bestDist) { bestDist = path.size(); best = &b; }
    }

    if (!best) return false;

    return moveUnitToward(unit, uGrid, buildingGridOf(**best, ts), reachable, mapManager, tc);
}

bool AIController::tryBlockProductionBuilding(Unit& unit, MapManager& mapManager, TurnController& tc) {
    // Only triggered when no alive enemy capture unit exists
    bool hasConquerUnit = false;
    for (const auto& u : mapManager.getUnits()) {
        if (!u->isDead() && u->getTeam() == Team::Enemy && u->hasFlag(UnitFlag::Capture)) {
            hasConquerUnit = true;
            break;
        }
    }
    if (hasConquerUnit) return false;

    sf::Vector2u ts    = mapManager.getTileSize();
    sf::Vector2i uGrid = gridOf(unit, ts);
    auto reachable = mapManager.getReachableTiles(uGrid, unit.getMoveSpeed(), unit.getTeam(), unit.getMovementCategory());

    const std::shared_ptr<Building>* best = nullptr;
    size_t bestDist = 99999;

    for (const auto& b : mapManager.getBuildings()) {
        if (b->getTeam() != Team::Ally) continue;
        const std::string& type = b->getTypeName();
        if (type != "Factory" && type != "HQ") continue;
        sf::Vector2i bGrid = buildingGridOf(*b, ts);
        auto occupant = mapManager.getUnitAtTile(bGrid);
        if (occupant && occupant->getTeam() == Team::Enemy) continue;
        
        auto path = mapManager.findPath(uGrid, bGrid, unit.getTeam(), unit.getMovementCategory());
        if (path.empty()) continue;
        
        if (path.size() < bestDist) { bestDist = path.size(); best = &b; }
    }

    if (!best) return false;

    return moveUnitToward(unit, uGrid, buildingGridOf(**best, ts), reachable, mapManager, tc);
}

bool AIController::tryAttackUnit(Unit& unit, MapManager& mapManager, TurnController& tc) {
    sf::Vector2u ts    = mapManager.getTileSize();
    sf::Vector2i uGrid = gridOf(unit, ts);
    int maxR = unit.getMaxAttackRange();
    int minR = unit.getMinAttackRange();

    auto inRange = [&](sf::Vector2i from, sf::Vector2i to) {
        int dx = std::abs(to.x - from.x), dy = std::abs(to.y - from.y);
        int d = (maxR == 1) ? (dx + dy) : std::max(dx, dy);
        return d >= minR && d <= maxR;
    };

    auto reachable = mapManager.getReachableTiles(uGrid, unit.getMoveSpeed(), unit.getTeam(), unit.getMovementCategory());

    struct Candidate {
        std::shared_ptr<Unit> target;
        std::vector<sf::Vector2i> movePath;
        sf::Vector2i attackFrom;
    };

    std::vector<Candidate> candidates;

    for (const auto& ally : mapManager.getUnits()) {
        if (ally->isDead() || ally->getTeam() != Team::Ally) continue;
        if (!unit.canTarget(*ally)) continue;
        sf::Vector2i aGrid = gridOf(*ally, ts);

        if (inRange(uGrid, aGrid)) {
            candidates.push_back({ ally, {}, uGrid });
            continue;
        }

        // Find closest reachable tile in attack range
        std::vector<sf::Vector2i> bestPath;
        sf::Vector2i bestTile = uGrid;
        for (const auto& tile : reachable) {
            if (!inRange(tile, aGrid)) continue;
            auto path = mapManager.findPath(uGrid, tile, unit.getTeam(), unit.getMovementCategory());
            if (!path.empty() && (bestPath.empty() || path.size() < bestPath.size())) {
                bestPath = path;
                bestTile = tile;
            }
        }
        if (!bestPath.empty())
            candidates.push_back({ ally, bestPath, bestTile });
    }

    if (candidates.empty()) return false;

    // Prefer target with lowest health
    auto& best = *std::min_element(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) {
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

bool AIController::tryMoveTowardPlayer(Unit& unit, MapManager& mapManager, TurnController& tc) {
    sf::Vector2u ts    = mapManager.getTileSize();
    sf::Vector2i uGrid = gridOf(unit, ts);
    auto reachable = mapManager.getReachableTiles(uGrid, unit.getMoveSpeed(), unit.getTeam(), unit.getMovementCategory());

    // Find nearest ally unit by path distance
    const std::shared_ptr<Unit>* nearest = nullptr;
    size_t bestDist = 99999;
    for (const auto& ally : mapManager.getUnits()) {
        if (ally->isDead() || ally->getTeam() != Team::Ally) continue;
        if (!unit.canTarget(*ally)) continue;
        sf::Vector2i aGrid = gridOf(*ally, ts);
        
        auto path = mapManager.findPath(uGrid, aGrid, unit.getTeam(), unit.getMovementCategory());
        if (path.empty()) continue; // No path available
        
        if (path.size() < bestDist) { 
            bestDist = path.size(); 
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
