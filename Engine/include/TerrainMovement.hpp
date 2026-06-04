#pragma once

enum class TerrainType { Grass, Water, Mountain, Road, Forest };

enum class MovementCategory { Ground, Infantry, Flying, Naval };

constexpr bool canTraverse(TerrainType terrain, MovementCategory category) {
  switch (terrain) {
  case TerrainType::Grass:
    return category != MovementCategory::Naval;
  case TerrainType::Water:
    return category == MovementCategory::Flying ||
           category == MovementCategory::Naval;
  case TerrainType::Mountain:
    return category == MovementCategory::Infantry ||
           category == MovementCategory::Flying;
  case TerrainType::Road:
    return category != MovementCategory::Naval;
  case TerrainType::Forest:
    return category == MovementCategory::Infantry ||
           category == MovementCategory::Flying;
  }
  return false;
}
