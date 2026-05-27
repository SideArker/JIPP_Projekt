#pragma once

#include "EngineAPI.hpp"
#include "Tile.hpp"
#include "Unit.hpp"
#include <string>
#include <vector>

struct ENGINE_API UnitSpawnData {
    std::string typeName;
    int gridX;
    int gridY;
    Team team;
};

struct ENGINE_API MapFile {
    std::string  tilesetPath;
    sf::Vector2u tileSize;
    unsigned int width  = 0;
    unsigned int height = 0;
    std::vector<Tile>          tiles;
    std::vector<UnitSpawnData> spawns;
};
