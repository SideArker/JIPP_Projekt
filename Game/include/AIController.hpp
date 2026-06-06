#pragma once

#include "CameraController.hpp"
#include "MapManager.hpp"
#include "TurnController.hpp"
#include "Unit.hpp"
#include <memory>
#include <vector>

class AIController {
public:
    void init(Team team) { m_myTeam = team; }
    void update(float dt, MapManager& mapManager, TurnController& tc, CameraController& camera);
    void reset();
    bool isDone() const { return m_started && m_queue.empty() && m_productionEvaluated && m_productionQueue.empty() && m_waitTimer <= 0.f; }

private:
    std::vector<std::shared_ptr<Unit>> m_queue;
    float m_waitTimer = 0.f;
    bool  m_started   = false;
    Team  m_myTeam    = Team::Enemy;

    static constexpr float ACTION_DELAY = 0.6f;

    struct ProductionTask {
        std::shared_ptr<Building> factory;
        std::string unitName;
        int cost;
    };
    std::vector<ProductionTask> m_productionQueue;
    bool m_productionEvaluated = false;

    void buildQueue(MapManager& mapManager);
    void processNextUnit(MapManager& mapManager, TurnController& tc, CameraController& camera);
    void evaluateProduction(MapManager& mapManager);
    void processProduction(MapManager& mapManager, TurnController& tc, CameraController& camera);

    bool tryConquerNeutral(Unit& unit, MapManager& mapManager, TurnController& tc);
    bool tryCapturePlayerBuilding(Unit& unit, MapManager& mapManager, TurnController& tc);
    bool tryBlockProductionBuilding(Unit& unit, MapManager& mapManager, TurnController& tc);
    bool tryAttackUnit(Unit& unit, MapManager& mapManager, TurnController& tc);
    bool tryMoveTowardPlayer(Unit& unit, MapManager& mapManager, TurnController& tc);

    bool isTileBlocking(sf::Vector2i tile, const Unit& unit, MapManager& mapManager) const;

    static bool moveUnitToward(Unit& unit, sf::Vector2i from, sf::Vector2i target,
                               const std::vector<sf::Vector2i>& reachable,
                               MapManager& mapManager, TurnController& tc);
};
