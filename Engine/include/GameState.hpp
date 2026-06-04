#pragma once

#include "EngineAPI.hpp"
#include "Unit.hpp"
#include <cstdint>
#include <string>
#include <vector>

struct ENGINE_API UnitSaveData {
  std::string typeName;
  int gridX = 0;
  int gridY = 0;
  int health = 0;
  int damage = 0;
  int moveSpeed = 0;
  Team team = Team::Neutral;
  uint8_t flags = 0;
};

struct ENGINE_API BuildingSaveData {
  std::string typeName;
  int gridX = 0;
  int gridY = 0;
  Team team = Team::Neutral;
};

struct ENGINE_API GameState {
  std::string mapFilePath;
  std::vector<UnitSaveData> units;
  std::vector<BuildingSaveData> buildings;
};
