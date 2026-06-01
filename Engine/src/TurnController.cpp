#include "TurnController.hpp"

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
