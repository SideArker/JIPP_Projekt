#pragma once

#include "CameraController.hpp"
#include "MapManager.hpp"
#include "TurnController.hpp"
#include "Unit.hpp"
#include <memory>
#include <vector>

class AIController {
public:
    void update(float dt, MapManager& mapManager, TurnController& tc, CameraController& camera);
    void reset();
    bool isDone() const { return m_started && m_queue.empty(); }

private:
    std::vector<std::shared_ptr<Unit>> m_queue;
    float m_waitTimer = 0.f;
    bool  m_started   = false;

    static constexpr float ACTION_DELAY = 0.6f;

    void buildQueue(MapManager& mapManager);
    void processNextUnit(MapManager& mapManager, TurnController& tc, CameraController& camera);

    bool tryConquerNeutral(Unit& unit, MapManager& mapManager, TurnController& tc);
    bool tryCapturePlayerBuilding(Unit& unit, MapManager& mapManager, TurnController& tc);
    bool tryBlockProductionBuilding(Unit& unit, MapManager& mapManager, TurnController& tc);
    bool tryAttackUnit(Unit& unit, MapManager& mapManager, TurnController& tc);
    bool tryMoveTowardPlayer(Unit& unit, MapManager& mapManager, TurnController& tc);

    static bool moveUnitToward(Unit& unit, sf::Vector2i from, sf::Vector2i target,
                               const std::vector<sf::Vector2i>& reachable,
                               MapManager& mapManager, TurnController& tc);
};
