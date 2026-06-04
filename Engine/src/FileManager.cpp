#include "FileManager.hpp"
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>

static constexpr uint32_t FILE_VERSION = 4;
static constexpr char MAP_MAGIC[4] = {'J', 'M', 'A', 'P'};
static constexpr char SAVE_MAGIC[4] = {'J', 'S', 'A', 'V'};

template <typename T> static void writeVal(std::ostream &out, const T &val) {
  out.write(reinterpret_cast<const char *>(&val), sizeof(T));
}

template <typename T> static bool readVal(std::istream &in, T &val) {
  in.read(reinterpret_cast<char *>(&val), sizeof(T));
  return in.good();
}

static void writeString(std::ostream &out, const std::string &str) {
  auto len = static_cast<uint16_t>(str.size());
  writeVal(out, len);
  out.write(str.data(), len);
}

static bool readString(std::istream &in, std::string &out) {
  uint16_t len;
  if (!readVal(in, len))
    return false;
  out.resize(len);
  in.read(out.data(), len);
  return in.good();
}

static bool validateMagic(std::istream &in, const char *expected) {
  char magic[4];
  in.read(magic, 4);
  return in.good() && std::memcmp(magic, expected, 4) == 0;
}

static void writeMapBlock(std::ostream &out, const MapFile &map) {
  writeString(out, map.tilesetPath);
  writeVal(out, map.tileSize.x);
  writeVal(out, map.tileSize.y);
  writeVal(out, map.width);
  writeVal(out, map.height);

  writeVal(out, static_cast<uint32_t>(map.tiles.size()));
  for (const auto &tile : map.tiles) {
    writeVal(out, static_cast<int32_t>(tile.getArtId()));
    writeVal(out, static_cast<uint8_t>(tile.getTerrain()));
    writeVal(out, tile.getRotation());
  }

  writeVal(out, static_cast<uint32_t>(map.spawns.size()));
  for (const auto &spawn : map.spawns) {
    writeString(out, spawn.typeName);
    writeVal(out, static_cast<int32_t>(spawn.gridX));
    writeVal(out, static_cast<int32_t>(spawn.gridY));
    writeVal(out, static_cast<uint8_t>(spawn.team));
    writeVal(out, static_cast<uint8_t>(spawn.facingDirection));
  }

  writeVal(out, static_cast<uint32_t>(map.buildingSpawns.size()));
  for (const auto &spawn : map.buildingSpawns) {
    writeString(out, spawn.typeName);
    writeVal(out, static_cast<int32_t>(spawn.gridX));
    writeVal(out, static_cast<int32_t>(spawn.gridY));
    writeVal(out, static_cast<uint8_t>(spawn.team));
  }

  writeVal(out, static_cast<uint32_t>(map.teams.size()));
  for (const auto &td : map.teams) {
    writeVal(out, static_cast<uint8_t>(td.team));
    writeString(out, td.name);
    writeVal(out, td.color.r);
    writeVal(out, td.color.g);
    writeVal(out, td.color.b);
    writeVal(out, td.color.a);
    writeVal(out, static_cast<int32_t>(td.startMoney));
  }
}

static bool readMapBlock(std::istream &in, MapFile &map, uint32_t version) {
  if (!readString(in, map.tilesetPath))
    return false;
  if (!readVal(in, map.tileSize.x))
    return false;
  if (!readVal(in, map.tileSize.y))
    return false;
  if (!readVal(in, map.width))
    return false;
  if (!readVal(in, map.height))
    return false;

  uint32_t tileCount;
  if (!readVal(in, tileCount))
    return false;
  map.tiles.reserve(tileCount);
  for (uint32_t i = 0; i < tileCount; ++i) {
    int32_t artId;
    uint8_t terrain;
    unsigned char rotation = 0;
    if (!readVal(in, artId) || !readVal(in, terrain))
      return false;
    if (version >= 3) {
      if (!readVal(in, rotation)) return false;
    }
    map.tiles.emplace_back(static_cast<int>(artId),
                           static_cast<TerrainType>(terrain), rotation);
  }

  uint32_t spawnCount;
  if (!readVal(in, spawnCount))
    return false;
  map.spawns.reserve(spawnCount);
  for (uint32_t i = 0; i < spawnCount; ++i) {
    UnitSpawnData s;
    int32_t gx, gy;
    uint8_t t;
    if (!readString(in, s.typeName))
      return false;
    if (!readVal(in, gx) || !readVal(in, gy) || !readVal(in, t))
      return false;
    s.gridX = gx;
    s.gridY = gy;
    s.team = static_cast<Team>(t);
    if (version >= 4) {
      uint8_t dir;
      if (!readVal(in, dir)) return false;
      s.facingDirection = static_cast<MoveDirection>(dir);
    } else {
      s.facingDirection = MoveDirection::Right;
    }
    map.spawns.push_back(s);
  }

  uint32_t buildingCount;
  if (!readVal(in, buildingCount))
    return false;
  map.buildingSpawns.resize(buildingCount);
  for (auto &spawn : map.buildingSpawns) {
    if (!readString(in, spawn.typeName))
      return false;
    int32_t gridX, gridY;
    uint8_t team;
    if (!readVal(in, gridX) || !readVal(in, gridY) || !readVal(in, team))
      return false;
    spawn.gridX = gridX;
    spawn.gridY = gridY;
    spawn.team = static_cast<Team>(team);
  }

  uint32_t teamCount;
  if (!readVal(in, teamCount))
    return false;
  map.teams.resize(teamCount);
  for (auto &td : map.teams) {
    uint8_t teamId;
    if (!readVal(in, teamId))
      return false;
    td.team = static_cast<Team>(teamId);
    if (!readString(in, td.name))
      return false;
    if (!readVal(in, td.color.r))
      return false;
    if (!readVal(in, td.color.g))
      return false;
    if (!readVal(in, td.color.b))
      return false;
    if (!readVal(in, td.color.a))
      return false;
    int32_t money;
    if (!readVal(in, money))
      return false;
    td.startMoney = money;
  }
  return true;
}

static void ensureParentExists(const std::string &path) {
  std::filesystem::path p(path);
  if (p.has_parent_path())
    std::filesystem::create_directories(p.parent_path());
}

bool FileManager::saveMap(const MapFile &map, const std::string &path) {
  ensureParentExists(path);
  std::ofstream out(path, std::ios::binary);
  if (!out)
    return false;

  out.write(MAP_MAGIC, 4);
  writeVal(out, FILE_VERSION);
  writeMapBlock(out, map);
  return out.good();
}

bool FileManager::loadMap(const std::string &path, MapFile &out) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return false;

  if (!validateMagic(in, MAP_MAGIC))
    return false;

  uint32_t version;
  if (!readVal(in, version) || version > FILE_VERSION)
    return false;

  return readMapBlock(in, out, version);
}

bool FileManager::saveGame(const GameState &state, const std::string &path) {
  ensureParentExists(path);
  std::ofstream out(path, std::ios::binary);
  if (!out)
    return false;

  out.write(SAVE_MAGIC, 4);
  writeVal(out, FILE_VERSION);
  writeVal(out, static_cast<uint32_t>(std::time(nullptr)));
  writeString(out, state.mapFilePath);

  writeVal(out, static_cast<uint32_t>(state.units.size()));
  for (const auto &unit : state.units) {
    writeString(out, unit.typeName);
    writeVal(out, static_cast<int32_t>(unit.gridX));
    writeVal(out, static_cast<int32_t>(unit.gridY));
    writeVal(out, static_cast<int32_t>(unit.health));
    writeVal(out, static_cast<int32_t>(unit.damage));
    writeVal(out, static_cast<int32_t>(unit.moveSpeed));
    writeVal(out, static_cast<uint8_t>(unit.team));
    writeVal(out, unit.flags);
  }
  writeVal(out, static_cast<uint32_t>(state.buildings.size()));
  for (const auto &b : state.buildings) {
    writeString(out, b.typeName);
    writeVal(out, static_cast<int32_t>(b.gridX));
    writeVal(out, static_cast<int32_t>(b.gridY));
    writeVal(out, static_cast<uint8_t>(b.team));
  }
  return out.good();
}

bool FileManager::loadGame(const std::string &path, GameState &out) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return false;

  if (!validateMagic(in, SAVE_MAGIC))
    return false;

  uint32_t version;
  if (!readVal(in, version) || version != FILE_VERSION)
    return false;

  uint32_t timestamp;
  if (!readVal(in, timestamp))
    return false;
  if (!readString(in, out.mapFilePath))
    return false;

  uint32_t unitCount;
  if (!readVal(in, unitCount))
    return false;
  out.units.resize(unitCount);

  for (auto &unit : out.units) {
    if (!readString(in, unit.typeName))
      return false;
    int32_t gridX, gridY, health, damage, moveSpeed;
    uint8_t team, flags;
    if (!readVal(in, gridX) || !readVal(in, gridY))
      return false;
    if (!readVal(in, health) || !readVal(in, damage))
      return false;
    if (!readVal(in, moveSpeed) || !readVal(in, team))
      return false;
    if (!readVal(in, flags))
      return false;
    unit.gridX = gridX;
    unit.gridY = gridY;
    unit.health = health;
    unit.damage = damage;
    unit.moveSpeed = moveSpeed;
    unit.team = static_cast<Team>(team);
    unit.flags = flags;
  }
  uint32_t buildingCount;
  if (!readVal(in, buildingCount))
    return false;
  out.buildings.resize(buildingCount);
  for (auto &b : out.buildings) {
    if (!readString(in, b.typeName))
      return false;
    int32_t gridX, gridY;
    uint8_t team;
    if (!readVal(in, gridX) || !readVal(in, gridY) || !readVal(in, team))
      return false;
    b.gridX = gridX;
    b.gridY = gridY;
    b.team = static_cast<Team>(team);
  }
  return true;
}
