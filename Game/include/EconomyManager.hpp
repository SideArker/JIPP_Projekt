#pragma once

#include "MapManager.hpp"
#include "Building.hpp"
#include "TeamRegistry.hpp"

class EconomyManager {
public:
    static void processTurnEnd(MapManager& mapManager);
};
