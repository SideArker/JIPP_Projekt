#pragma once
#include "MapFile.hpp"
#include "TerrainMovement.hpp"
#include "Unit.hpp"
#include <string>


static constexpr const char *DEFAULT_MAP_PATH =
    "../../../../Art/levels/level1.map";

enum class EditorTool { Tiles, Units, Buildings };

std::string teamLabel(Team team);
Team parseTeam(const std::string &id);
TerrainType parseTerrain(const std::string &id);
std::string terrainLabel(TerrainType t);
TerrainType terrainForTileId(int id);
std::string tileDescription(int id);
MapFile createDefaultMap();
