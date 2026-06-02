#include "MapEditorTypes.hpp"

std::string teamLabel(Team team) {
    switch (team) {
    case Team::Ally:     return "Ally";
    case Team::Enemy:    return "Enemy";
    case Team::Neutral:  return "Neutral";
    }
    return "Neutral";
}

Team parseTeam(const std::string& id) {
    if (id == "Ally")   return Team::Ally;
    if (id == "Enemy")  return Team::Enemy;
    return Team::Neutral;
}

TerrainType parseTerrain(const std::string& id) {
    if (id == "Water")    return TerrainType::Water;
    if (id == "Mountain") return TerrainType::Mountain;
    if (id == "Road")     return TerrainType::Road;
    if (id == "Forest")   return TerrainType::Forest;
    return TerrainType::Grass;
}

std::string terrainLabel(TerrainType t) {
    switch (t) {
    case TerrainType::Water:    return "Water";
    case TerrainType::Mountain: return "Mountain";
    case TerrainType::Road:     return "Road";
    case TerrainType::Forest:   return "Forest";
    default:                    return "Grass";
    }
}

TerrainType terrainForTileId(int id) {
    switch (id) {
    case 6:  return TerrainType::Water;
    case 7:  return TerrainType::Mountain;
    case 17: return TerrainType::Mountain;
    case 10: return TerrainType::Road;
    case 11: return TerrainType::Road;
    case 12: return TerrainType::Road;
    case 16: return TerrainType::Road;
    case 2: return TerrainType::Grass;
    case 3: return TerrainType::Grass;
    case 4: return TerrainType::Grass;
    case 5: return TerrainType::Grass;
    case 8: return TerrainType::Grass;
    case 9: return TerrainType::Grass;
    case 13: return TerrainType::Grass;
    case 14: return TerrainType::Grass;
    case 15: return TerrainType::Grass;
    case 18: return TerrainType::Grass;
    case 19: return TerrainType::Grass;
    default:
        return TerrainType::Grass;
    }
}

std::string tileDescription(int id) {
    switch (id) {
    case  2: return " - Grass";
    case  3: return " - Grass2";
    case  4: return " - Grass3";
    case  5: return " - Grass right barrier";
    case  6: return " - Water";
    case  7: return " - Mountain";
    case  8: return " - Grass bottom-right barrier";
    case  9: return " - Grass top-right barrier";
    case 10: return " - Road left/right";
    case 11: return " - T-Road";
    case 12: return " - Road up/down";
    case 13: return " - Grass barrier left";
    case 14: return " - Grass bottom-left barrier";
    case 15: return " - Grass bottom";
    case 16: return " - Road turn";
    case 17: return " - Mountain2";
    case 18: return " - Grass top-left barrier";
    case 19: return " - Grass top barrier";
    default: return "";
    }
}

MapFile createDefaultMap() {
    MapFile map;
    map.tilesetPath = "Art/Map/map.png";
    map.tileSize    = {32, 32};
    map.width       = 24;
    map.height      = 16;
    map.tiles.assign(map.width * map.height, Tile(19, TerrainType::Grass));
    return map;
}
