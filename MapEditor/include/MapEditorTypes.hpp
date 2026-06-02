#pragma once
#include <string>
#include "MapFile.hpp"
#include "TerrainMovement.hpp"
#include "Unit.hpp"

static constexpr const char* DEFAULT_MAP_PATH = "levels/editor.map";

enum class EditorTool {
    Tiles,
    Units,
    Buildings
};

std::string teamLabel(Team team);
Team        parseTeam(const std::string& id);
TerrainType parseTerrain(const std::string& id);
std::string terrainLabel(TerrainType t);
TerrainType terrainForTileId(int id);
std::string tileDescription(int id);
MapFile     createDefaultMap();
