#include "MapEditorTypes.hpp"

std::string teamLabel(Team team) {
  switch (team) {
  case Team::Ally:
    return "Ally";
  case Team::Enemy:
    return "Enemy";
  case Team::Neutral:
    return "Neutral";
  }
  return "Neutral";
}

Team parseTeam(const std::string &id) {
  if (id == "Ally")
    return Team::Ally;
  if (id == "Enemy")
    return Team::Enemy;
  return Team::Neutral;
}

TerrainType parseTerrain(const std::string &id) {
  if (id == "Water")
    return TerrainType::Water;
  if (id == "Mountain")
    return TerrainType::Mountain;
  if (id == "Road")
    return TerrainType::Road;
  if (id == "Forest")
    return TerrainType::Forest;
  return TerrainType::Grass;
}

std::string terrainLabel(TerrainType t) {
  switch (t) {
  case TerrainType::Water:
    return "Water";
  case TerrainType::Mountain:
    return "Mountain";
  case TerrainType::Road:
    return "Road";
  case TerrainType::Forest:
    return "Forest";
  default:
    return "Grass";
  }
}

TerrainType terrainForTileId(int id) {
  switch (id) {
  case 5:
    return TerrainType::Mountain;
  case 6:
    return TerrainType::Mountain;
  case 7:
    return TerrainType::Road;
  case 8:
    return TerrainType::Road;
  case 9:
    return TerrainType::Road;
  case 10:
    return TerrainType::Road;
  case 11:
    return TerrainType::Water;
  case 12:
    return TerrainType::Grass;
  case 13:
    return TerrainType::Grass;
  case 14:
    return TerrainType::Grass;
  case 15:
    return TerrainType::Grass;
  case 16:
    return TerrainType::Grass;
  case 17:
    return TerrainType::Grass;
  case 18:
    return TerrainType::Grass;
  case 19:
    return TerrainType::Grass;
  default:
    return TerrainType::Grass;
  }
}

std::string tileDescription(int id) {
  switch (id) {
  case 1:
    return " - Grey Checkerboard";
  case 2:
    return " - Plain Grass";
  case 3:
    return " - Grass small debris";
  case 4:
    return " - Grass small craters";
  case 5:
    return " - Mountain 1";
  case 6:
    return " - Large Rock";
  case 7:
    return " - Road left";
  case 8:
    return " - Asphalt Road Corner";
  case 9:
    return " - Asphalt Road Center";
  case 10:
    return " - Asphalt Road Horizontal";
  case 11:
    return " - Pure Water";
  case 12:
    return " - Coast Right Water";
  case 13:
    return " - Coast Left Water";
  case 14:
    return " - Coast Bottom Water";
  case 15:
    return " - Coast Top Water";
  case 16:
    return " - Coast Bottom-Right Water";
  case 17:
    return " - Coast Top-Right Water";
  case 18:
    return " - Coast Bottom-Left Water";
  case 19:
    return " - Coast Top-Left Water";
  default:
    return "";
  }
}

MapFile createDefaultMap() {
  MapFile map;
  map.tilesetPath = "Art/Map/map.png";
  map.tileSize = {32, 32};
  map.width = 24;
  map.height = 16;
  map.tiles.assign(map.width * map.height, Tile(2, TerrainType::Grass));

  TeamData td1;
  td1.team = Team::Ally;
  td1.name = "Blue Nation";
  td1.color = sf::Color(50, 150, 255);
  td1.startMoney = 1500;

  TeamData td2;
  td2.team = Team::Enemy;
  td2.name = "Red Empire";
  td2.color = sf::Color(255, 50, 50);
  td2.startMoney = 1500;
  td2.isAi = true;

  map.teams = {td1, td2};
  return map;
}
