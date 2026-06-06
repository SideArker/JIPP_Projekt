#pragma once

#include "EngineAPI.hpp"
#include "Unit.hpp"
#include <array>
#include <memory>
#include <vector>
#include <optional>

class Building;

class ENGINE_API TurnController {
public:
    Team getCurrentTeam() const { return m_turnOrder[m_teamIndex]; }
    int  getTurnNumber()  const { return m_turnNumber; }

    bool canAct(const Unit& unit) const;
    void markActed(Unit& unit);
    void endTurn(std::vector<std::shared_ptr<Unit>>& units);
    std::optional<Team> checkWinCondition(const std::vector<std::shared_ptr<Unit>>& units, 
                                          const std::vector<std::shared_ptr<Building>>& buildings) const;

    void setTurnOrder(const std::vector<Team>& teams);

private:
    std::vector<Team> m_turnOrder;
    int m_teamIndex  = 0;
    int m_turnNumber = 1;
};
