#pragma once

#include "EngineAPI.hpp"
#include "Tile.hpp"
#include "Unit.hpp"
#include <SFML/Graphics/Color.hpp>
#include <string>
#include <vector>

struct ENGINE_API TeamData {
    Team team  = Team::Neutral;
    std::string name;
    sf::Color color = sf::Color::White;
};

struct ENGINE_API UnitSpawnData {
    std::string typeName;
    int gridX;
    int gridY;
    Team team;
};

struct ENGINE_API BuildingSpawnData {
    std::string typeName;
    int gridX = 0;
    int gridY = 0;
    Team team = Team::Neutral;
};

struct ENGINE_API MapFile {
    std::string  tilesetPath;
    sf::Vector2u tileSize;
    unsigned int width = 0;
    unsigned int height = 0;
    std::vector<Tile> tiles;
    std::vector<UnitSpawnData> spawns;
    std::vector<BuildingSpawnData> buildingSpawns;
    std::vector<TeamData> teams;
};
