#include "TurnController.hpp"
#include "Building.hpp"

bool TurnController::canAct(const Unit& unit) const {
    return unit.getTeam() == getCurrentTeam() && !unit.hasActed() && !unit.isDead();
}

void TurnController::markActed(Unit& unit) {
    unit.setActed(true);
}

void TurnController::setTurnOrder(const std::vector<Team>& teams) {
    m_turnOrder = teams;
    m_teamIndex = 0;
    m_turnNumber = 1;
}

void TurnController::endTurn(std::vector<std::shared_ptr<Unit>>& units) {
    if (m_turnOrder.empty()) return;
    m_teamIndex = (m_teamIndex + 1) % static_cast<int>(m_turnOrder.size());
    if (m_teamIndex == 0) ++m_turnNumber;
    for (auto& u : units)
        if (!u->isDead()) u->setActed(false);
}

std::optional<Team> TurnController::checkWinCondition(const std::vector<std::shared_ptr<Unit>>& units, 
                                                      const std::vector<std::shared_ptr<Building>>& buildings) const {
    std::vector<Team> aliveTeams;

    bool mapHasAnyHq = false;
    for (const auto& b : buildings) {
        if (b->getTypeName() == "HQ") {
            mapHasAnyHq = true;
            break;
        }
    }

    for (Team t : m_turnOrder) {
        bool hasHq = false;
        bool hasUnit = false;
        for (const auto& b : buildings) {
            if (b->getTypeName() == "HQ" && b->getTeam() == t) {
                hasHq = true;
                break;
            }
        }
        for (const auto& u : units) {
            if (!u->isDead() && u->getTeam() == t) {
                hasUnit = true;
                break;
            }
        }

        if (hasUnit && (hasHq || !mapHasAnyHq)) {
            aliveTeams.push_back(t);
        }
    }

    if (aliveTeams.size() == 1) {
        return aliveTeams[0];
    }

    return std::nullopt;
}
