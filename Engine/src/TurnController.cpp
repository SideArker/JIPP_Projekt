#include "TurnController.hpp"
#include "Building.hpp"

bool TurnController::canAct(const Unit& unit) const {
    return unit.getTeam() == getCurrentTeam() && !unit.hasActed() && !unit.isDead();
}

void TurnController::markActed(Unit& unit) {
    unit.setActed(true);
}

void TurnController::endTurn(std::vector<std::shared_ptr<Unit>>& units) {
    m_teamIndex = (m_teamIndex + 1) % static_cast<int>(m_turnOrder.size());
    if (m_teamIndex == 0) ++m_turnNumber;
    for (auto& u : units)
        if (!u->isDead()) u->setActed(false);
}

std::optional<Team> TurnController::checkWinCondition(const std::vector<std::shared_ptr<Unit>>& units, 
                                                      const std::vector<std::shared_ptr<Building>>& buildings) const {
    bool allyHQExists = false;
    bool enemyHQExists = false;
    for (const auto& b : buildings) {
        if (b->getTypeName() == "HQ") {
            if (b->getTeam() == Team::Ally) allyHQExists = true;
            if (b->getTeam() == Team::Enemy) enemyHQExists = true;
        }
    }

    bool enemyHasUnits = false;
    bool allyHasUnits = false;
    for (const auto& u : units) {
        if (!u->isDead()) {
            if (u->getTeam() == Team::Enemy) enemyHasUnits = true;
            if (u->getTeam() == Team::Ally) allyHasUnits = true;
        }
    }

    bool allyWon = false;
    bool enemyWon = false;

    if (!enemyHQExists || !enemyHasUnits) allyWon = true;
    if (!allyHQExists || !allyHasUnits) enemyWon = true;

    if (allyWon && !enemyWon) return Team::Ally;
    if (enemyWon && !allyWon) return Team::Enemy;

    return std::nullopt;
}
