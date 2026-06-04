#include "MapManager.hpp"
#include "BuildingRegistry.hpp"
#include "FileManager.hpp"
#include "MapFile.hpp"
#include "SelectionController.hpp"
#include "SoundManager.hpp"
#include "TeamRegistry.hpp"
#include "TextureManager.hpp"
#include "UnitRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <unordered_map>

// Core gameplay file that manages the map, units, buildings, and turn logic.

struct Node {
  sf::Vector2i pos;
  float gCost = 0.f;
  float hCost = 0.f;
  float fCost = 0.f;
  sf::Vector2i parent;

  bool operator>(const Node &other) const {
    if (fCost == other.fCost)
      return hCost > other.hCost;
    return fCost > other.fCost;
  }
};

static constexpr float kCaptureJump1Duration = 0.34f;
static constexpr float kCaptureJump2Duration = 0.22f;
static constexpr float kCaptureJump3Duration = 0.15f;

static constexpr float kCaptureJump1Amplitude = 13.0f;
static constexpr float kCaptureJump2Amplitude = 7.0f;
static constexpr float kCaptureJump3Amplitude = 3.5f;

static constexpr float kCaptureBounceDuration =
    kCaptureJump1Duration + kCaptureJump2Duration + kCaptureJump3Duration;
static constexpr float kPi = 3.14159265f;

float getCaptureBounceOffsetY(float remainingTime) {
  if (remainingTime <= 0.f || kCaptureBounceDuration <= 0.f) {
    return 0.f;
  }

  const float elapsed = std::clamp(kCaptureBounceDuration - remainingTime, 0.f,
                                   kCaptureBounceDuration);

  if (elapsed < kCaptureJump1Duration) {
    return -kCaptureJump1Amplitude *
           std::sin(kPi * elapsed / kCaptureJump1Duration);
  }

  if (elapsed < (kCaptureJump1Duration + kCaptureJump2Duration)) {
    const float local = elapsed - kCaptureJump1Duration;
    return -kCaptureJump2Amplitude *
           std::sin(kPi * local / kCaptureJump2Duration);
  }

  const float local = elapsed - kCaptureJump1Duration - kCaptureJump2Duration;
  return -kCaptureJump3Amplitude *
         std::sin(kPi * local / kCaptureJump3Duration);
}

MapManager::MapManager() : mapWidth(0), mapHeight(0) {
  if (!m_uiFont.openFromFile("Art/Fonts/joystixMonospace.ttf")) {
    // Warning or log could go here
  }
}

MapManager::~MapManager() = default;

bool MapManager::loadMap(const std::string &tileset, sf::Vector2u tileSize,
                         const std::vector<Tile> &tiles, unsigned int w,
                         unsigned int h) {
  this->tilesetPath = tileset;
  this->tileSize = tileSize;
  mapData = tiles;
  mapWidth = w;
  mapHeight = h;
  return renderer.load(tileset, tileSize, tiles, w, h);
}

void MapManager::spawnUnit(std::shared_ptr<Unit> unit, int gridX, int gridY) {
  unit->setPosition(sf::Vector2f(static_cast<float>(gridX * tileSize.x),
                                 static_cast<float>(gridY * tileSize.y)));
  auto weakUnit = std::weak_ptr<Unit>(unit);
  std::string typeName = unit->getName();

  const UnitData *data = UnitRegistry::getData(typeName);
  float attackDelay = data ? data->attackDamageDelay : 0.75f;
  float hitDelay = unit->getHitEffectDelay();
  std::string deathSet = data ? data->deathEffectSet : "";
  std::string deathClip = data ? data->deathEffectClip : "";
  std::string deathTex = data ? data->deathEffectTexturePath : "";
  std::string deathSSet = data ? data->deathSoundSet : "";
  std::string deathSName = data ? data->deathSoundName : "";

  unit->onAttackStart = [this, typeName, hitDelay, attackDelay,
                         unit](std::shared_ptr<Unit> target, int dmg) {
    SoundManager::play(typeName, "shoot");
    sf::Vector2f targetPos = target->getPosition();
    if (hitDelay <= 0.f) {
      spawnHitEffect(targetPos);
      SoundManager::play(typeName, "hit");
    } else {
      m_pendingActions.push_back({hitDelay, [this, typeName, targetPos]() {
                                    spawnHitEffect(targetPos);
                                    SoundManager::play(typeName, "hit");
                                  }});
    }
    m_pendingActions.push_back({hitDelay + attackDelay, [target, dmg, unit]() {
                                  if (!target->isDead())
                                    target->takeDamage(dmg);
                                }});
  };

  unit->onDamaged = [this, weakUnit, deathSet, deathClip, deathTex, deathSSet,
                     deathSName](sf::Vector2f pos, int health) {
    if (health <= 0) {
      auto u = weakUnit.lock();
      if (u)
        spawnDeathEffect(deathSet, deathClip, deathTex, deathSSet, deathSName,
                         pos, u);
    }
  };

  units.push_back(unit);
}

void MapManager::spawnBuilding(std::shared_ptr<Building> building, int gridX,
                               int gridY) {
  building->setPosition(sf::Vector2f(static_cast<float>(gridX * tileSize.x),
                                     static_cast<float>(gridY * tileSize.y)));
  buildings.push_back(building);
}

std::shared_ptr<Building>
MapManager::getBuildingAtTile(sf::Vector2i gridPos) const {
  for (const auto &building : buildings) {
    sf::Vector2i bGrid(
        static_cast<int>(std::round(building->getPosition().x /
                                    static_cast<float>(tileSize.x))),
        static_cast<int>(std::round(building->getPosition().y /
                                    static_cast<float>(tileSize.y))));
    if (bGrid == gridPos)
      return building;
  }
  return nullptr;
}

void MapManager::setupInput(sf::RenderWindow &window) {
  selectionController = std::make_unique<SelectionController>(
      window, *this, m_turnController, m_walkOverlayPath, m_moveArrowPath,
      m_iconsPath, m_enemyOverlayPath);
  SoundManager::playMusic("AllyTurn");
  SoundManager::setMusicVolume(20.f);
}

void MapManager::handleEvent(const sf::Event &event) {
  if (selectionController)
    selectionController->handleEvent(event);
}

void MapManager::update(float deltaTime) {
  auto currentSelected = getSelectedUnit();
  if (m_lastSelectedUnit != currentSelected) {
    m_lastSelectedUnit = currentSelected;
    notifySelectionChanged(currentSelected, nullptr, nullptr);
  }
  for (auto &unit : units)
    unit->update(deltaTime);
  for (auto &e : m_effects) {
    e.animState.update(deltaTime);
    e.sprite.setTextureRect(e.animState.getCurrentRect());
  }
  for (auto &e : m_effects) {
    if (e.animState.isFinished() && e.onFinished) {
      e.onFinished();
      e.onFinished = nullptr;
    }
  }
  m_effects.erase(
      std::remove_if(m_effects.begin(), m_effects.end(),
                     [](const Effect &e) { return e.animState.isFinished(); }),
      m_effects.end());
  for (auto &[timer, action] : m_pendingActions)
    timer -= deltaTime;
  for (auto &[timer, action] : m_pendingActions)
    if (timer <= 0.f && action) {
      action();
      action = nullptr;
    }
  m_pendingActions.erase(
      std::remove_if(m_pendingActions.begin(), m_pendingActions.end(),
                     [](const auto &p) { return p.first <= 0.f; }),
      m_pendingActions.end());

  if (!isAnyUnitActing() && !m_whenIdleActions.empty()) {
    auto callbacks = std::move(m_whenIdleActions);
    m_whenIdleActions.clear();
    for (auto &cb : callbacks) {
      if (cb)
        cb();
    }
  }

  if (m_captureBounceTimer > 0.f) {
    m_captureBounceTimer = std::max(0.f, m_captureBounceTimer - deltaTime);
    if (m_captureBounceTimer <= 0.f) {
      m_captureBounceTargets.clear();
    }
  }

  m_uiTime += deltaTime;
}

void MapManager::draw(sf::RenderTarget &target) {
  target.draw(renderer);

  // Draw Game Border (Black and Orange striped)
  float bw = static_cast<float>(mapWidth * tileSize.x);
  float bh = static_cast<float>(mapHeight * tileSize.y);
  float t = 4.f;       // border thickness
  float segLen = 16.f; // stripe segment length

  auto addRect = [&](float x, float y, float w, float h, sf::Color c) {
    sf::RectangleShape rect({w, h});
    rect.setPosition({x, y});
    rect.setFillColor(c);
    target.draw(rect);
  };

  // Top and Bottom edges
  for (float x = 0; x < bw; x += segLen) {
    sf::Color c = (static_cast<int>(x / segLen) % 2 == 0)
                      ? sf::Color::Black
                      : sf::Color(255, 128, 0);
    float w = std::min(segLen, bw - x);
    addRect(x, -t, w, t, c); // Top
    addRect(x, bh, w, t, c); // Bottom
  }
  // Left and Right edges
  for (float y = 0; y < bh; y += segLen) {
    sf::Color c = (static_cast<int>(y / segLen) % 2 == 0)
                      ? sf::Color::Black
                      : sf::Color(255, 128, 0);
    float h = std::min(segLen, bh - y);
    addRect(-t, y, t, h, c); // Left
    addRect(bw, y, t, h, c); // Right
  }

  for (const auto &building : buildings) {
    sf::Sprite sprite(building->getTexture());
    if (building->hasTextureRect())
      sprite.setTextureRect(building->getTextureRect());
    sprite.setPosition(building->getPosition());
    target.draw(sprite);
  }
  if (selectionController)
    selectionController->drawOverlays(target);

  bool acting = isAnyUnitActing();
  for (const auto &unit : units) {
    if (unit->isDead())
      continue;
    sf::Sprite unitSprite(unit->getCurrentTexture());
    sf::IntRect rect = unit->getCurrentRect();
    unitSprite.setTextureRect(rect);
    if (unit->hasActed() && !unit->isActing())
      unitSprite.setColor(sf::Color(150, 150, 150));
    if (unit->shouldFlipX()) {
      unitSprite.setScale({-1.f, 1.f});
      unitSprite.setPosition(
          {unit->getPosition().x + static_cast<float>(rect.size.x),
           unit->getPosition().y});
    } else {
      unitSprite.setPosition(unit->getPosition());
    }
    target.draw(unitSprite);

    if (m_unitRenderCallback)
      m_unitRenderCallback(target, *unit, acting);
  }

  if (!m_teamCaptureTexturePath.empty()) {
    static constexpr float kOverlayOffY = -5.f;
    static constexpr int kCellW = 32;

    for (const auto &building : buildings) {
      int progress = building->getCaptureProgress();
      if (progress <= 0)
        continue;

      float bounceY = 0.f;
      if (m_captureBounceTimer > 0.f &&
          m_captureBounceTargets.find(building.get()) !=
              m_captureBounceTargets.end()) {
        bounceY = getCaptureBounceOffsetY(m_captureBounceTimer);
      }

      sf::Vector2i bGrid(
          static_cast<int>(std::round(building->getPosition().x /
                                      static_cast<float>(tileSize.x))),
          static_cast<int>(std::round(building->getPosition().y /
                                      static_cast<float>(tileSize.y))));
      auto occupant = getUnitAtTile(bGrid);
      if (!occupant || occupant->isDead())
        continue;

      sf::IntRect rect = occupant->getCurrentRect();
      sf::Sprite bouncedUnit(occupant->getCurrentTexture());
      bouncedUnit.setTextureRect(rect);
      if (occupant->hasActed() && !occupant->isActing())
        bouncedUnit.setColor(sf::Color(150, 150, 150));
      const float ux = occupant->getPosition().x;
      const float uy = occupant->getPosition().y + bounceY;
      if (occupant->shouldFlipX()) {
        bouncedUnit.setScale({-1.f, 1.f});
        bouncedUnit.setPosition({ux + static_cast<float>(rect.size.x), uy});
      } else {
        bouncedUnit.setPosition({ux, uy});
      }
      target.draw(bouncedUnit);

      const sf::Color teamColor =
          TeamRegistry::getColor(building->getCaptureTeam());
      const sf::Texture &capTex = TextureManager::getTexture(
          m_teamCaptureTexturePath, m_teamCaptureMaskPath, teamColor);

      sf::Sprite capSprite(capTex);
      const int cellIdx = std::clamp(progress - 1, 0, 2);
      capSprite.setTextureRect(
          sf::IntRect({cellIdx * kCellW, 0}, {kCellW, kCellW}));

      const float tx = building->getPosition().x;
      const float ty = building->getPosition().y;

      const float capX =
          tx +
          (static_cast<float>(tileSize.x) - static_cast<float>(kCellW)) * 0.5f;
      capSprite.setPosition({capX, ty + kOverlayOffY + bounceY});
      target.draw(capSprite);
    }
  }

  for (const auto &e : m_effects)
    target.draw(e.sprite);
  if (selectionController)
    selectionController->drawCursorIcon(target);
}

void MapManager::drawUI(sf::RenderWindow &window) {
  if (selectionController)
    selectionController->drawGui();

  if (m_turnController.getCurrentTeam() == Team::Enemy) {
    sf::View savedView = window.getView();
    window.setView(window.getDefaultView());

    sf::Text text(m_uiFont, "ENEMY TURN", 40u);

    // oscillating alpha
    float alpha = (std::sin(m_uiTime * 4.f) + 1.f) * 0.5f * 255.f;
    text.setFillColor(sf::Color(255, 50, 50, static_cast<std::uint8_t>(alpha)));
    text.setOutlineColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alpha)));
    text.setOutlineThickness(2.f);

    sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({bounds.size.x / 1.5f, bounds.size.y / 2.f});
    text.setPosition({window.getSize().x / 2.f, 60.f});

    window.draw(text);
    window.setView(savedView);
  }
}

void MapManager::setHitEffect(std::string setName, std::string clipName,
                              std::string texturePath) {
  m_hitEffectSet = std::move(setName);
  m_hitEffectClip = std::move(clipName);
  m_hitEffectTexturePath = std::move(texturePath);
}

void MapManager::setTeamCaptureEffect(std::string texturePath,
                                      std::string maskPath) {
  m_teamCaptureTexturePath = std::move(texturePath);
  m_teamCaptureMaskPath = std::move(maskPath);
}

void MapManager::setUnitRenderCallback(
    std::function<void(sf::RenderTarget &, const Unit &, bool)> cb) {
  m_unitRenderCallback = std::move(cb);
}

void MapManager::endTurn() {
  const bool resolveCaptureNow =
      (m_turnController.getCurrentTeam() == Team::Enemy);
  std::unordered_set<const Building *> progressed;

  if (resolveCaptureNow) {
    for (const auto &building : buildings) {
      sf::Vector2i bGrid(
          static_cast<int>(std::round(building->getPosition().x /
                                      static_cast<float>(tileSize.x))),
          static_cast<int>(std::round(building->getPosition().y /
                                      static_cast<float>(tileSize.y))));
      auto occupant = getUnitAtTile(bGrid);
      if (building->onTurnEnd(occupant.get())) {
        progressed.insert(building.get());
      }
    }
  }

  m_undoStack.clear();
  m_turnController.endTurn(units);

  if (!progressed.empty()) {
    m_captureBounceTargets = std::move(progressed);
    m_captureBounceTimer = kCaptureBounceDuration;
  } else {
    m_captureBounceTargets.clear();
    m_captureBounceTimer = 0.f;
  }

  if (m_turnController.getCurrentTeam() == Team::Ally) {
    SoundManager::playMusic("AllyTurn");
    std::cout << "Ally Turn" << std::endl;
  } else {
    std::cout << "Enemy Turn" << std::endl;
    SoundManager::playMusic("EnemyTurn");
  }
  SoundManager::setMusicVolume(20.f);
}

Team MapManager::getCurrentTeam() const {
  return m_turnController.getCurrentTeam();
}

TurnController &MapManager::getTurnController() { return m_turnController; }

void MapManager::requestEndTurn() {
  if (selectionController)
    selectionController->clearSelection();
  endTurn();
}

tgui::Gui *MapManager::getGui() {
  return selectionController ? &selectionController->getGui() : nullptr;
}

void MapManager::syncCameraView(const sf::View &gameView) {
  if (selectionController)
    selectionController->syncCameraView(gameView);
}

void MapManager::setWalkOverlayPath(std::string path) {
  m_walkOverlayPath = std::move(path);
}

void MapManager::setMoveArrowPath(std::string path) {
  m_moveArrowPath = std::move(path);
}

void MapManager::setIconsPath(std::string path) {
  m_iconsPath = std::move(path);
}

void MapManager::setEnemyOverlayPath(std::string path) {
  m_enemyOverlayPath = std::move(path);
}

void MapManager::spawnEffect(const std::string &setName,
                             const std::string &clipName,
                             const std::string &texturePath,
                             sf::Vector2f position, float yOffset) {
  const AnimationSet *set = AnimationManager::getSet(setName);
  if (!set)
    return;
  auto it = m_effectTextures.find(texturePath);
  if (it == m_effectTextures.end()) {
    sf::Texture tex;
    if (!tex.loadFromFile(texturePath))
      return;
    it = m_effectTextures.emplace(texturePath, std::move(tex)).first;
  }
  Effect e{sf::Sprite(it->second)};
  e.animSet = set;
  e.animState.play(clipName, *set);
  e.sprite.setTextureRect(e.animState.getCurrentRect());
  e.sprite.setPosition({position.x, position.y + yOffset});
  m_effects.push_back(std::move(e));
}

void MapManager::spawnHitEffect(sf::Vector2f position) {
  spawnEffect(m_hitEffectSet, m_hitEffectClip, m_hitEffectTexturePath, position,
              0.f);
}

void MapManager::spawnDeathEffect(const std::string &setName,
                                  const std::string &clipName,
                                  const std::string &texturePath,
                                  const std::string &soundSet,
                                  const std::string &soundName,
                                  sf::Vector2f position,
                                  std::shared_ptr<Unit> unit) {
  units.erase(std::remove(units.begin(), units.end(), unit), units.end());
  if (!soundSet.empty())
    SoundManager::play(soundSet, soundName);
  if (!setName.empty())
    spawnEffect(setName, clipName, texturePath, position, -32.f);
}

sf::Vector2u MapManager::getTileSize() const { return tileSize; }
unsigned int MapManager::getMapWidth() const { return mapWidth; }
unsigned int MapManager::getMapHeight() const { return mapHeight; }

bool MapManager::isAnyUnitActing() const {
  return std::any_of(
             units.begin(), units.end(),
             [](const std::shared_ptr<Unit> &u) { return u->isActing(); }) ||
         !m_pendingActions.empty() || !m_effects.empty();
}

void MapManager::runWhenAllActionsFinished(std::function<void()> action) {
  if (!action)
    return;
  if (!isAnyUnitActing()) {
    action();
    return;
  }
  m_whenIdleActions.push_back(std::move(action));
}

std::shared_ptr<Unit> MapManager::getUnitAtTile(sf::Vector2i gridPos) const {
  for (const auto &unit : units) {
    if (unit->isDead())
      continue;
    sf::Vector2i unitGrid(
        static_cast<int>(
            std::round(unit->getPosition().x / static_cast<float>(tileSize.x))),
        static_cast<int>(std::round(unit->getPosition().y /
                                    static_cast<float>(tileSize.y))));
    if (unitGrid == gridPos) {
      return unit;
    }
  }
  return nullptr;
}

static float getTerrainCost(TerrainType type) {
  switch (type) {
  case TerrainType::Road: return 0.5f;
  case TerrainType::Mountain: return 2.0f;
  case TerrainType::Grass:
  case TerrainType::Forest:
  case TerrainType::Water:
  default: return 1.0f;
  }
}

std::vector<sf::Vector2i>
MapManager::getReachableTiles(sf::Vector2i from, float moveRange, Team movingTeam,
                              MovementCategory category) const {
  std::vector<sf::Vector2i> reachable;
  std::unordered_map<int, float> minCost;
  struct PQNode {
    float cost;
    sf::Vector2i pos;
    bool operator>(const PQNode& other) const { return cost > other.cost; }
  };
  std::priority_queue<PQNode, std::vector<PQNode>, std::greater<PQNode>> pq;

  pq.push({0.f, from});
  minCost[from.x + from.y * static_cast<int>(mapWidth)] = 0.f;

  const std::vector<sf::Vector2i> directions = {
      {0, -1}, {0, 1}, {-1, 0}, {1, 0}};

  while (!pq.empty()) {
    auto [cost, pos] = pq.top();
    pq.pop();

    if (cost > minCost[pos.x + pos.y * static_cast<int>(mapWidth)])
      continue;

    if (pos != from) {
      auto occupant = getUnitAtTile(pos);
      if (occupant == nullptr || occupant->getTeam() != movingTeam) {
        reachable.push_back(pos);
      }
    }

    for (const auto &dir : directions) {
      sf::Vector2i next = pos + dir;
      if (next.x < 0 || next.y < 0 || next.x >= static_cast<int>(mapWidth) ||
          next.y >= static_cast<int>(mapHeight))
        continue;
      auto terrain = mapData[next.x + next.y * static_cast<int>(mapWidth)].getTerrain();
      if (!canTraverse(terrain, category))
        continue;
      auto occupant = getUnitAtTile(next);
      if (occupant != nullptr && occupant->getTeam() != movingTeam)
        continue;

      float nextCost = cost + getTerrainCost(terrain);
      if (nextCost > moveRange)
        continue;

      int key = next.x + next.y * static_cast<int>(mapWidth);
      if (minCost.find(key) == minCost.end() || nextCost < minCost[key]) {
        minCost[key] = nextCost;
        pq.push({nextCost, next});
      }
    }
  }
  return reachable;
}

static float getManhattanDistance(sf::Vector2i a, sf::Vector2i b) {
  return static_cast<float>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
}

std::vector<sf::Vector2i> MapManager::findPath(sf::Vector2i start,
                                               sf::Vector2i goal,
                                               Team movingTeam,
                                               MovementCategory category) {
  std::vector<sf::Vector2i> path;
  auto isValid = [&](int x, int y) {
    if (x < 0 || x >= static_cast<int>(mapWidth) || y < 0 ||
        y >= static_cast<int>(mapHeight))
      return false;
    return canTraverse(mapData[x + y * static_cast<int>(mapWidth)].getTerrain(),
                       category);
  };

  if (!isValid(start.x, start.y) || !isValid(goal.x, goal.y)) {
    return path;
  }

  std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
  std::unordered_map<int, Node> allNodes;

  Node startNode = {start, 0.f, getManhattanDistance(start, goal), 0.f, start};
  startNode.fCost = startNode.gCost + startNode.hCost;

  openSet.push(startNode);
  allNodes[start.x + start.y * static_cast<int>(mapWidth)] = startNode;

  const std::vector<sf::Vector2i> directions = {
      {0, -1}, {0, 1}, {-1, 0}, {1, 0}};

  while (!openSet.empty()) {
    Node current = openSet.top();
    openSet.pop();

    if (current.pos == goal) {
      sf::Vector2i currPos = goal;
      while (currPos != start) {
        path.push_back(currPos);
        currPos =
            allNodes[currPos.x + currPos.y * static_cast<int>(mapWidth)].parent;
      }
      std::reverse(path.begin(), path.end());
      return path;
    }

    for (const auto &dir : directions) {
      sf::Vector2i neighborPos = current.pos + dir;
      if (!isValid(neighborPos.x, neighborPos.y))
        continue;
      auto occupant = getUnitAtTile(neighborPos);
      if (occupant != nullptr && occupant->getTeam() != movingTeam)
        continue;

      auto terrain = mapData[neighborPos.x + neighborPos.y * static_cast<int>(mapWidth)].getTerrain();
      float newGCost = current.gCost + getTerrainCost(terrain);
      int neighborKey =
          neighborPos.x + neighborPos.y * static_cast<int>(mapWidth);

      if (allNodes.find(neighborKey) == allNodes.end() ||
          newGCost < allNodes[neighborKey].gCost) {
        Node neighborNode;
        neighborNode.pos = neighborPos;
        neighborNode.gCost = newGCost;
        neighborNode.hCost = getManhattanDistance(neighborPos, goal);
        neighborNode.fCost = neighborNode.gCost + neighborNode.hCost;
        neighborNode.parent = current.pos;

        allNodes[neighborKey] = neighborNode;
        openSet.push(neighborNode);
      }
    }
  }
  return path;
}

bool MapManager::loadFromFile(const std::string &mapPath) {
  MapFile mapFile;
  if (!FileManager::loadMap(mapPath, mapFile))
    return false;

  units.clear();
  buildings.clear();
  selectionController.reset();
  currentMapPath = mapPath;

  m_teams.clear();
  for (const auto &td : mapFile.teams) {
    m_teams[td.team] = td;
  }
  if (m_teams.find(Team::Ally) == m_teams.end()) {
    m_teams[Team::Ally] = {Team::Ally, "Blue Team", sf::Color::Blue, 1000};
  }
  if (m_teams.find(Team::Enemy) == m_teams.end()) {
    m_teams[Team::Enemy] = {Team::Enemy, "Red Team", sf::Color::Red, 1000};
  }
  if (m_teams.find(Team::Neutral) == m_teams.end()) {
    m_teams[Team::Neutral] = {Team::Neutral, "Neutral",
                              sf::Color(128, 128, 128), 0};
  }

  if (!loadMap(mapFile.tilesetPath, mapFile.tileSize, mapFile.tiles,
               mapFile.width, mapFile.height))
    return false;

  for (const auto &spawn : mapFile.spawns) {
    auto unit = UnitRegistry::create(spawn.typeName, spawn.team);
    if (unit) {
      unit->setDirection(spawn.facingDirection);
      spawnUnit(unit, spawn.gridX, spawn.gridY);
    }
  }
  for (const auto &spawn : mapFile.buildingSpawns) {
    auto building = BuildingRegistry::create(spawn.typeName, spawn.team);
    if (building)
      spawnBuilding(building, spawn.gridX, spawn.gridY);
  }
  return true;
}

bool MapManager::saveToFile(const std::string &mapPath) const {
  MapFile mapFile;
  mapFile.tilesetPath = tilesetPath;
  mapFile.tileSize = tileSize;
  mapFile.width = mapWidth;
  mapFile.height = mapHeight;
  mapFile.tiles = mapData;

  for (const auto &[team, data] : m_teams) {
    mapFile.teams.push_back(data);
  }

  for (const auto &unit : units) {
    auto gridPos = unit->getGridPosition(tileSize);
    mapFile.spawns.push_back({unit->getName(), gridPos.x, gridPos.y,
                              unit->getTeam(), unit->getDirection()});
  }
  for (const auto &building : buildings) {
    sf::Vector2i gridPos(
        static_cast<int>(std::round(building->getPosition().x /
                                    static_cast<float>(tileSize.x))),
        static_cast<int>(std::round(building->getPosition().y /
                                    static_cast<float>(tileSize.y))));
    mapFile.buildingSpawns.push_back(
        {building->getTypeName(), gridPos.x, gridPos.y, building->getTeam()});
  }
  return FileManager::saveMap(mapFile, mapPath);
}

GameState MapManager::captureGameState() const {
  GameState state;
  state.mapFilePath = currentMapPath;

  for (const auto &unit : units) {
    auto gridPos = unit->getGridPosition(tileSize);
    state.units.push_back(
        {unit->getName(), gridPos.x, gridPos.y, unit->getHealth(),
         unit->getDamage(), unit->getMoveSpeed(),
         unit->getTeam(), unit->getFlags(), unit->hasActed()});
  }
  for (const auto &building : buildings) {
    sf::Vector2i gridPos(
        static_cast<int>(std::round(building->getPosition().x /
                                    static_cast<float>(tileSize.x))),
        static_cast<int>(std::round(building->getPosition().y /
                                    static_cast<float>(tileSize.y))));
    state.buildings.push_back(
        {building->getTypeName(), gridPos.x, gridPos.y, building->getTeam()});
  }
  return state;
}

bool MapManager::restoreGameState(const std::string &savePath) {
  GameState state;
  if (!FileManager::loadGame(savePath, state))
    return false;
  return restoreGameState(state);
}

bool MapManager::restoreGameState(const GameState &state) {
  MapFile mapFile;
  if (!FileManager::loadMap(state.mapFilePath, mapFile))
    return false;

  units.clear();
  buildings.clear();
  if (selectionController)
    selectionController->clearSelection();
  currentMapPath = state.mapFilePath;

  if (!loadMap(mapFile.tilesetPath, mapFile.tileSize, mapFile.tiles,
               mapFile.width, mapFile.height))
    return false;

  for (const auto &unitData : state.units) {
    auto unit = UnitRegistry::create(unitData.typeName, unitData.team);
    if (!unit)
      continue;
    unit->setHealth(unitData.health);
    unit->setDamage(unitData.damage);
    unit->setMoveSpeed(unitData.moveSpeed);
    unit->setFlags(unitData.flags);
    unit->setActed(unitData.hasActed);
    unit->setDirection(
        MoveDirection::Right); // Default or maybe we should save it
    spawnUnit(unit, unitData.gridX, unitData.gridY);
  }
  for (const auto &bData : state.buildings) {
    auto building = getBuildingAtTile({bData.gridX, bData.gridY});
    if (building)
      building->setTeam(bData.team);
  }
  return true;
}

void MapManager::pushUndoState(sf::Vector2i unitGrid) {
  UndoState s;
  s.state = captureGameState();
  s.unitGrid = unitGrid;
  m_undoStack.push_back(std::move(s));
}

bool MapManager::popUndoState(std::shared_ptr<Unit> &outSelectedUnit) {
  if (m_undoStack.empty())
    return false;
  UndoState s = std::move(m_undoStack.back());
  m_undoStack.pop_back();

  if (restoreGameState(s.state)) {
    outSelectedUnit = getUnitAtTile(s.unitGrid);
    return true;
  }
  return false;
}

void MapManager::clearUndoStack() { m_undoStack.clear(); }

std::shared_ptr<Unit> MapManager::getSelectedUnit() const {
  if (selectionController) {
    return selectionController->getSelectedUnit();
  }
  return nullptr;
}

void MapManager::selectUnit(std::shared_ptr<Unit> unit) {
  if (selectionController)
    selectionController->selectUnit(unit);
}