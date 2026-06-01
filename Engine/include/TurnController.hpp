#pragma once

#include "EngineAPI.hpp"
#include "Unit.hpp"
#include <array>
#include <memory>
#include <vector>

class ENGINE_API TurnController {
public:
    Team getCurrentTeam() const { return m_turnOrder[m_teamIndex]; }
    int  getTurnNumber()  const { return m_turnNumber; }

    bool canAct(const Unit& unit) const;
    void markActed(Unit& unit);
    void endTurn(std::vector<std::shared_ptr<Unit>>& units);

private:
    std::array<Team, 2> m_turnOrder = { Team::Ally, Team::Enemy };
    int m_teamIndex  = 0;
    int m_turnNumber = 1;
};
