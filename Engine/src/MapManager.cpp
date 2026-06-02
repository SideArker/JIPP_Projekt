#include "MapManager.hpp"
#include "SelectionController.hpp"
#include "FileManager.hpp"
#include "MapFile.hpp"
#include "UnitRegistry.hpp"
#include "BuildingRegistry.hpp"
#include "SoundManager.hpp"
#include "TeamRegistry.hpp"
#include "TextureManager.hpp"
#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <iostream>

// Core gameplay file that manages the map, units, buildings, and turn logic.

struct Node {
    sf::Vector2i pos;
    int gCost = 0;
    int hCost = 0;
    int fCost = 0;
    sf::Vector2i parent;

    bool operator>(const Node& other) const {
        if (fCost == other.fCost) return hCost > other.hCost;
        return fCost > other.fCost;
    }
};

MapManager::MapManager() : mapWidth(0), mapHeight(0) {}

MapManager::~MapManager() = default;

bool MapManager::loadMap(const std::string& tileset, sf::Vector2u tileSize, const std::vector<Tile>& tiles, unsigned int w, unsigned int h) {
    this->tilesetPath = tileset;
    this->tileSize = tileSize;
    mapData = tiles;
    mapWidth = w;
    mapHeight = h;
    return renderer.load(tileset, tileSize, tiles, w, h);
}

void MapManager::spawnUnit(std::shared_ptr<Unit> unit, int gridX, int gridY) {
    unit->setPosition(sf::Vector2f(
        static_cast<float>(gridX * tileSize.x),
        static_cast<float>(gridY * tileSize.y)
    ));
    auto weakUnit = std::weak_ptr<Unit>(unit);
    std::string typeName = unit->getName();

    const UnitData* data = UnitRegistry::getData(typeName);
    float attackDelay = data ? data->attackDamageDelay: 0.75f;
    float hitDelay = unit->getHitEffectDelay();
    std::string deathSet = data ? data->deathEffectSet: "";
    std::string deathClip = data ? data->deathEffectClip: "";
    std::string deathTex = data ? data->deathEffectTexturePath: "";
    std::string deathSSet = data ? data->deathSoundSet: "";
    std::string deathSName = data ? data->deathSoundName: "";

    unit->onAttackStart = [this, typeName, hitDelay, attackDelay, unit](std::shared_ptr<Unit> target, int dmg) {
        SoundManager::play(typeName, "shoot");
        sf::Vector2f targetPos = target->getPosition();
        if (hitDelay <= 0.f) {
            spawnHitEffect(targetPos);
            SoundManager::play(typeName, "hit");
        } else {
            m_pendingActions.push_back({ hitDelay, [this, typeName, targetPos]() {
                spawnHitEffect(targetPos);
                SoundManager::play(typeName, "hit");
            } });
        }
        m_pendingActions.push_back({ hitDelay + attackDelay, [target, dmg, unit]() {
            if (!target->isDead())
                target->takeDamage(dmg);
        } });
    };

    unit->onDamaged = [this, weakUnit, deathSet, deathClip, deathTex, deathSSet, deathSName](sf::Vector2f pos, int health) {
        if (health <= 0) {
            auto u = weakUnit.lock();
            if (u)
                spawnDeathEffect(deathSet, deathClip, deathTex, deathSSet, deathSName, pos, u);
        }
    };

    units.push_back(unit);
}

void MapManager::spawnBuilding(std::shared_ptr<Building> building, int gridX, int gridY) {
    building->setPosition(sf::Vector2f(
        static_cast<float>(gridX * tileSize.x),
        static_cast<float>(gridY * tileSize.y)
    ));
    buildings.push_back(building);
}

std::shared_ptr<Building> MapManager::getBuildingAtTile(sf::Vector2i gridPos) const {
    for (const auto& building : buildings) {
        sf::Vector2i bGrid(
            static_cast<int>(std::round(building->getPosition().x / static_cast<float>(tileSize.x))),
            static_cast<int>(std::round(building->getPosition().y / static_cast<float>(tileSize.y)))
        );
        if (bGrid == gridPos) return building;
    }
    return nullptr;
}

void MapManager::setupInput(sf::RenderWindow& window) {
    selectionController = std::make_unique<SelectionController>(window, *this, m_turnController, m_walkOverlayPath, m_moveArrowPath, m_iconsPath, m_enemyOverlayPath);
}

void MapManager::handleEvent(const sf::Event& event) {
    if (selectionController) selectionController->handleEvent(event);
}

void MapManager::update(float deltaTime) {
    for (auto& unit : units)
        unit->update(deltaTime);
    for (auto& e : m_effects) {
        e.animState.update(deltaTime);
        e.sprite.setTextureRect(e.animState.getCurrentRect());
    }
    for (auto& e : m_effects) {
        if (e.animState.isFinished() && e.onFinished) {
            e.onFinished();
            e.onFinished = nullptr;
        }
    }
    m_effects.erase(
        std::remove_if(m_effects.begin(), m_effects.end(),
            [](const Effect& e) { return e.animState.isFinished(); }),
        m_effects.end()
    );
    for (auto& [timer, action] : m_pendingActions)
        timer -= deltaTime;
    for (auto& [timer, action] : m_pendingActions)
        if (timer <= 0.f && action) { action(); action = nullptr; }
    m_pendingActions.erase(
        std::remove_if(m_pendingActions.begin(), m_pendingActions.end(),
            [](const auto& p) { return p.first <= 0.f; }),
        m_pendingActions.end()
    );

    if (!isAnyUnitActing() && !m_whenIdleActions.empty()) {
        auto callbacks = std::move(m_whenIdleActions);
        m_whenIdleActions.clear();
        for (auto& cb : callbacks) {
            if (cb) cb();
        }
    }

    m_captureBounceClock += deltaTime;
}

void MapManager::draw(sf::RenderTarget& target) {
    target.draw(renderer);
    for (const auto& building : buildings) {
        sf::Sprite sprite(building->getTexture());
        if (building->hasTextureRect())
            sprite.setTextureRect(building->getTextureRect());
        sprite.setPosition(building->getPosition());
        target.draw(sprite);
    }
    if (selectionController) selectionController->drawOverlays(target);

    bool acting = isAnyUnitActing();
    for (const auto& unit : units) {
        if (unit->isDead()) continue;
        sf::Sprite unitSprite(unit->getCurrentTexture());
        sf::IntRect rect = unit->getCurrentRect();
        unitSprite.setTextureRect(rect);
        if (unit->hasActed() && !unit->isActing())
            unitSprite.setColor(sf::Color(150, 150, 150));
        if (unit->shouldFlipX()) {
            unitSprite.setScale({ -1.f, 1.f });
            unitSprite.setPosition({ unit->getPosition().x + static_cast<float>(rect.size.x), unit->getPosition().y });
        } else {
            unitSprite.setPosition(unit->getPosition());
        }
        target.draw(unitSprite);

        // Start music
        SoundManager::playMusic("AllyTurn");
        if (m_unitRenderCallback)
            m_unitRenderCallback(target, *unit, acting);

    }

    // Draw capture bounce & TeamCapture overlays
    if (!m_teamCaptureTexturePath.empty()) {
        static constexpr float kOverlayOffY = -18.f;
        static constexpr int kCellW = 32;

        static constexpr float kPeriod = 2.2f;
        static constexpr float kAmp0 = 8.0f,  kDur0 = 0.55f;
        static constexpr float kAmp1 = 4.0f,  kDur1 = 0.30f;
        static constexpr float kAmp2 = 2.0f,  kDur2 = 0.15f;
        static constexpr float kPi = 3.14159265f;

        float bounceY = 0.f;
        {
            float phase = std::fmod(m_captureBounceClock, kPeriod);
            if (phase < kDur0)
                bounceY = -kAmp0 * std::sin(kPi * phase / kDur0);
            else if ((phase -= kDur0) < kDur1)
                bounceY = -kAmp1 * std::sin(kPi * phase / kDur1);
            else if ((phase -= kDur1) < kDur2)
                bounceY = -kAmp2 * std::sin(kPi * phase / kDur2);
        }

        for (const auto& building : buildings) {
            int progress = building->getCaptureProgress();
            if (progress <= 0) continue;  // not being captured

            sf::Vector2i bGrid(
                static_cast<int>(std::round(building->getPosition().x / static_cast<float>(tileSize.x))),
                static_cast<int>(std::round(building->getPosition().y / static_cast<float>(tileSize.y)))
            );
            auto occupant = getUnitAtTile(bGrid);
            if (!occupant || occupant->isDead()) continue;

            // Bounce animation
            sf::IntRect rect = occupant->getCurrentRect();
            sf::Sprite bouncedUnit(occupant->getCurrentTexture());
            bouncedUnit.setTextureRect(rect);
            if (occupant->hasActed() && !occupant->isActing())
                bouncedUnit.setColor(sf::Color(150, 150, 150));
            const float ux = occupant->getPosition().x;
            const float uy = occupant->getPosition().y + bounceY;
            if (occupant->shouldFlipX()) {
                bouncedUnit.setScale({ -1.f, 1.f });
                bouncedUnit.setPosition({ ux + static_cast<float>(rect.size.x), uy });
            } else {
                bouncedUnit.setPosition({ ux, uy });
            }
            target.draw(bouncedUnit);

            // TeamCapture overlay
            const sf::Color teamColor = TeamRegistry::getColor(building->getCaptureTeam());
            const sf::Texture& capTex = TextureManager::getTexture(
                m_teamCaptureTexturePath, m_teamCaptureMaskPath, teamColor);

            sf::Sprite capSprite(capTex);
            const int cellIdx = std::clamp(progress - 1, 0, 2);
            capSprite.setTextureRect(sf::IntRect({ cellIdx * kCellW, 0 }, { kCellW, kCellW }));

            const float tx = building->getPosition().x;
            const float ty = building->getPosition().y;

            const float capX = tx + (static_cast<float>(tileSize.x) - static_cast<float>(kCellW)) * 0.5f;
            capSprite.setPosition({ capX, ty + kOverlayOffY + bounceY });
            target.draw(capSprite);
        }
    }

    for (const auto& e : m_effects) target.draw(e.sprite);
    if (selectionController) selectionController->drawCursorIcon(target);
}

void MapManager::drawUI() {
    if (selectionController) selectionController->drawGui();
}

void MapManager::setHitEffect(std::string setName, std::string clipName, std::string texturePath) {
    m_hitEffectSet         = std::move(setName);
    m_hitEffectClip        = std::move(clipName);
    m_hitEffectTexturePath = std::move(texturePath);
}

void MapManager::setTeamCaptureEffect(std::string texturePath, std::string maskPath) {
    m_teamCaptureTexturePath = std::move(texturePath);
    m_teamCaptureMaskPath    = std::move(maskPath);
}

void MapManager::setUnitRenderCallback(std::function<void(sf::RenderTarget&, const Unit&, bool)> cb) {
    m_unitRenderCallback = std::move(cb);
}

void MapManager::endTurn() {
    for (const auto& building : buildings) {
        sf::Vector2i bGrid(
            static_cast<int>(std::round(building->getPosition().x / static_cast<float>(tileSize.x))),
            static_cast<int>(std::round(building->getPosition().y / static_cast<float>(tileSize.y)))
        );
        auto occupant = getUnitAtTile(bGrid);
        building->onTurnEnd(occupant.get());
    }
    m_turnController.endTurn(units);
    m_captureBounceClock = 0.f;
    if (m_turnController.getCurrentTeam() == Team::Ally)
    {
        SoundManager::playMusic("AllyTurn");
        std::cout << "Ally Turn" << std::endl;
    }
    else
    {std::cout << "Enemy Turn" << std::endl;
        SoundManager::playMusic("EnemyTurn");
    }
    SoundManager::setMusicVolume(20.f);
}

Team MapManager::getCurrentTeam() const {
    return m_turnController.getCurrentTeam();
}

TurnController& MapManager::getTurnController() {
    return m_turnController;
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

void MapManager::spawnEffect(const std::string& setName, const std::string& clipName, const std::string& texturePath, sf::Vector2f position, float yOffset) {
    const AnimationSet* set = AnimationManager::getSet(setName);
    if (!set) return;
    auto it = m_effectTextures.find(texturePath);
    if (it == m_effectTextures.end()) {
        sf::Texture tex;
        if (!tex.loadFromFile(texturePath)) return;
        it = m_effectTextures.emplace(texturePath, std::move(tex)).first;
    }
    Effect e{ sf::Sprite(it->second) };
    e.animSet = set;
    e.animState.play(clipName, *set);
    e.sprite.setTextureRect(e.animState.getCurrentRect());
    e.sprite.setPosition({ position.x, position.y + yOffset });
    m_effects.push_back(std::move(e));
}

void MapManager::spawnHitEffect(sf::Vector2f position) {
    spawnEffect(m_hitEffectSet, m_hitEffectClip, m_hitEffectTexturePath, position, 0.f);
}

void MapManager::spawnDeathEffect(const std::string& setName, const std::string& clipName, const std::string& texturePath, const std::string& soundSet, const std::string& soundName, sf::Vector2f position, std::shared_ptr<Unit> unit) {
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
    return std::any_of(units.begin(), units.end(),
        [](const std::shared_ptr<Unit>& u) { return u->isActing(); })
        || !m_pendingActions.empty()
        || !m_effects.empty();
}

void MapManager::runWhenAllActionsFinished(std::function<void()> action) {
    if (!action) return;
    if (!isAnyUnitActing()) {
        action();
        return;
    }
    m_whenIdleActions.push_back(std::move(action));
}



std::shared_ptr<Unit> MapManager::getUnitAtTile(sf::Vector2i gridPos) const {
    for (const auto& unit : units) {
        if (unit->isDead()) continue;
        sf::Vector2i unitGrid(
            static_cast<int>(std::round(unit->getPosition().x / static_cast<float>(tileSize.x))),
            static_cast<int>(std::round(unit->getPosition().y / static_cast<float>(tileSize.y)))
        );
        if (unitGrid == gridPos) {
            return unit;
        }
    }
    return nullptr;
}

std::vector<sf::Vector2i> MapManager::getReachableTiles(sf::Vector2i from, int moveRange, Team movingTeam, MovementCategory category) const {
    std::vector<sf::Vector2i> reachable;
    std::unordered_map<int, int> visited;
    std::queue<std::pair<sf::Vector2i, int>> bfsQueue;

    bfsQueue.push({ from, 0 });
    visited[from.x + from.y * static_cast<int>(mapWidth)] = 0;

    const std::vector<sf::Vector2i> directions = { {0,-1},{0,1},{-1,0},{1,0} };

    while (!bfsQueue.empty()) {
        auto [pos, steps] = bfsQueue.front();
        bfsQueue.pop();

        if (pos != from) {
            auto occupant = getUnitAtTile(pos);
            if (occupant == nullptr || occupant->getTeam() != movingTeam) {
                reachable.push_back(pos);
            }
        }

        if (steps >= moveRange) continue;

        for (const auto& dir : directions) {
            sf::Vector2i next = pos + dir;
            if (next.x < 0 || next.y < 0 ||
                next.x >= static_cast<int>(mapWidth) ||
                next.y >= static_cast<int>(mapHeight)) continue;
            if (!canTraverse(mapData[next.x + next.y * static_cast<int>(mapWidth)].getTerrain(), category)) continue;
            auto occupant = getUnitAtTile(next);
            if (occupant != nullptr && occupant->getTeam() != movingTeam) continue;
            int key = next.x + next.y * static_cast<int>(mapWidth);
            if (visited.find(key) == visited.end()) {
                visited[key] = steps + 1;
                bfsQueue.push({ next, steps + 1 });
            }
        }
    }
    return reachable;
}

static int getManhattanDistance(sf::Vector2i a, sf::Vector2i b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

std::vector<sf::Vector2i> MapManager::findPath(sf::Vector2i start, sf::Vector2i goal, Team movingTeam, MovementCategory category) {
    std::vector<sf::Vector2i> path;
    auto isValid = [&](int x, int y) {
        if (x < 0 || x >= static_cast<int>(mapWidth) || y < 0 || y >= static_cast<int>(mapHeight)) return false;
        return canTraverse(mapData[x + y * static_cast<int>(mapWidth)].getTerrain(), category);
    };

    if (!isValid(start.x, start.y) || !isValid(goal.x, goal.y)) {
        return path;
    }

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
    std::unordered_map<int, Node> allNodes;

    Node startNode = { start, 0, getManhattanDistance(start, goal), 0, start };
    startNode.fCost = startNode.gCost + startNode.hCost;

    openSet.push(startNode);
    allNodes[start.x + start.y * static_cast<int>(mapWidth)] = startNode;

    const std::vector<sf::Vector2i> directions = { {0,-1},{0,1},{-1,0},{1,0} };

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        if (current.pos == goal) {
            sf::Vector2i currPos = goal;
            while (currPos != start) {
                path.push_back(currPos);
                currPos = allNodes[currPos.x + currPos.y * static_cast<int>(mapWidth)].parent;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (const auto& dir : directions) {
            sf::Vector2i neighborPos = current.pos + dir;
            if (!isValid(neighborPos.x, neighborPos.y)) continue;
            auto occupant = getUnitAtTile(neighborPos);
            if (occupant != nullptr && occupant->getTeam() != movingTeam) continue;

            int newGCost = current.gCost + 10;
            int neighborKey = neighborPos.x + neighborPos.y * static_cast<int>(mapWidth);

            if (allNodes.find(neighborKey) == allNodes.end() || newGCost < allNodes[neighborKey].gCost) {
                Node neighborNode;
                neighborNode.pos = neighborPos;
                neighborNode.gCost = newGCost;
                neighborNode.hCost = getManhattanDistance(neighborPos, goal) * 10;
                neighborNode.fCost = neighborNode.gCost + neighborNode.hCost;
                neighborNode.parent = current.pos;
                allNodes[neighborKey] = neighborNode;
                openSet.push(neighborNode);
            }
        }
    }
    return path;
}

bool MapManager::loadFromFile(const std::string& mapPath) {
    MapFile mapFile;
    if (!FileManager::loadMap(mapPath, mapFile)) return false;

    units.clear();
    buildings.clear();
    selectionController.reset();
    currentMapPath = mapPath;

    if (!loadMap(mapFile.tilesetPath, mapFile.tileSize, mapFile.tiles, mapFile.width, mapFile.height))
        return false;

    for (const auto& spawn : mapFile.spawns) {
        auto unit = UnitRegistry::create(spawn.typeName, spawn.team);
        if (unit) spawnUnit(unit, spawn.gridX, spawn.gridY);
    }
    for (const auto& spawn : mapFile.buildingSpawns) {
        auto building = BuildingRegistry::create(spawn.typeName, spawn.team);
        if (building) spawnBuilding(building, spawn.gridX, spawn.gridY);
    }
    return true;
}

bool MapManager::saveToFile(const std::string& mapPath) const {
    MapFile mapFile;
    mapFile.tilesetPath = tilesetPath;
    mapFile.tileSize    = tileSize;
    mapFile.width       = mapWidth;
    mapFile.height      = mapHeight;
    mapFile.tiles       = mapData;

    for (const auto& unit : units) {
        auto gridPos = unit->getGridPosition(tileSize);
        mapFile.spawns.push_back({ unit->getName(), gridPos.x, gridPos.y, unit->getTeam() });
    }
    for (const auto& building : buildings) {
        sf::Vector2i gridPos(
            static_cast<int>(std::round(building->getPosition().x / static_cast<float>(tileSize.x))),
            static_cast<int>(std::round(building->getPosition().y / static_cast<float>(tileSize.y)))
        );
        mapFile.buildingSpawns.push_back({ building->getTypeName(), gridPos.x, gridPos.y, building->getTeam() });
    }
    return FileManager::saveMap(mapFile, mapPath);
}

GameState MapManager::captureGameState() const {
    GameState state;
    state.mapFilePath = currentMapPath;

    for (const auto& unit : units) {
        auto gridPos = unit->getGridPosition(tileSize);
        state.units.push_back({
            unit->getName(),
            gridPos.x,
            gridPos.y,
            unit->getHealth(),
            unit->getDamage(),
            static_cast<int>(unit->getMoveSpeed()),
            unit->getTeam(),
            unit->getFlags()
        });
    }
    for (const auto& building : buildings) {
        sf::Vector2i gridPos(
            static_cast<int>(std::round(building->getPosition().x / static_cast<float>(tileSize.x))),
            static_cast<int>(std::round(building->getPosition().y / static_cast<float>(tileSize.y)))
        );
        state.buildings.push_back({ building->getTypeName(), gridPos.x, gridPos.y, building->getTeam() });
    }
    return state;
}

bool MapManager::restoreGameState(const std::string& savePath) {
    GameState state;
    if (!FileManager::loadGame(savePath, state)) return false;

    MapFile mapFile;
    if (!FileManager::loadMap(state.mapFilePath, mapFile)) return false;

    units.clear();
    buildings.clear();
    selectionController.reset();
    currentMapPath = state.mapFilePath;

    if (!loadMap(mapFile.tilesetPath, mapFile.tileSize, mapFile.tiles, mapFile.width, mapFile.height))
        return false;

    for (const auto& unitData : state.units) {
        auto unit = UnitRegistry::create(unitData.typeName, unitData.team);
        if (!unit) continue;
        unit->setHealth(unitData.health);
        unit->setDamage(unitData.damage);
        unit->setMoveSpeed(unitData.moveSpeed);
        unit->setFlags(unitData.flags);
        spawnUnit(unit, unitData.gridX, unitData.gridY);
    }
    for (const auto& bData : state.buildings) {
        auto building = getBuildingAtTile({ bData.gridX, bData.gridY });
        if (building) building->setTeam(bData.team);
    }
    return true;
}
