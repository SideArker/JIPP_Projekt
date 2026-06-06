#include "EconomyManager.hpp"

void EconomyManager::processTurnEnd(MapManager& mapManager) {
    Team newTeam = mapManager.getTurnController().getCurrentTeam();
    if (mapManager.getTurnController().getTurnNumber() > 1) {
        for (const auto& building : mapManager.getBuildings()) {
            if (building->getTeam() == newTeam) {
                if (building->getTypeName() == "LandOilRig") {
                    mapManager.addTeamMoney(newTeam, 100);
                } else if (building->getTypeName() == "SeaOilRig") {
                    mapManager.addTeamMoney(newTeam, 200);
                }
            }
        }
    }
}
