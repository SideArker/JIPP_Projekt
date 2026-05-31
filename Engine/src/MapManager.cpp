#include "MapManager.hpp"
#include "SelectionController.hpp"
#include "FileManager.hpp"
#include "MapFile.hpp"
#include "UnitRegistry.hpp"
#include "SoundManager.hpp"
#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>

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

    unit->onAttackStart = [this, typeName, hitDelay = unit->getHitEffectDelay()](std::shared_ptr<Unit> target, int dmg) {
        SoundManager::play(typeName, "shoot");
        sf::Vector2f targetPos = target->getPosition();
        if (hitDelay <= 0.f) {
            spawnHitEffect(targetPos);
        } else {
            m_pendingActions.push_back({ hitDelay, [this, targetPos]() {
                spawnHitEffect(targetPos);
            } });
        }
        m_pendingActions.push_back({ m_attackDamageDelay, [target, dmg]() {
            if (!target->isDead())
                target->takeDamage(dmg);
        } });
        };

    unit->onDamaged = [this, weakUnit, typeName](sf::Vector2f pos, int health) {
        if (health <= 0) {
            auto u = weakUnit.lock();
            if (u)
                spawnExplosionEffect(pos, u);
        }
        };

    units.push_back(unit);
}

void MapManager::setupInput(sf::RenderWindow& window) {
    selectionController = std::make_unique<SelectionController>(window, *this);
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
}

void MapManager::draw(sf::RenderTarget& target) {
    target.draw(renderer);
    if (selectionController) selectionController->drawOverlays(target);

    if (!m_overlaysLoaded) {
        m_overlayFriendlyTexture.loadFromFile("Art/Effects/Unit_Overlay_Friendly.png");
        m_overlayEnemyTexture.loadFromFile("Art/Effects/Unit_Overlay_Enemy.png");
        m_overlaysLoaded = true;
    }

    for (const auto& unit : units) {
        if (unit->isDead()) continue;
        sf::Sprite unitSprite(unit->getCurrentTexture());
        sf::IntRect rect = unit->getCurrentRect();
        unitSprite.setTextureRect(rect);
        if (unit->shouldFlipX()) {
            unitSprite.setScale({ -1.f, 1.f });
            unitSprite.setPosition({ unit->getPosition().x + static_cast<float>(rect.size.x), unit->getPosition().y });
        } else {
            unitSprite.setPosition(unit->getPosition());
        }
        target.draw(unitSprite);

        if (!isAnyUnitActing()) {
            const sf::Texture& overlayTex = (unit->getTeam() == Team::Enemy)
                ? m_overlayEnemyTexture
                : m_overlayFriendlyTexture;
            sf::Sprite overlaySprite(overlayTex);
            overlaySprite.setPosition(unit->getPosition());
            target.draw(overlaySprite);
        }


        if (unit->getHealth() < unit->getMaxHealth()) {
            if (!m_healthTextureLoaded) {
                m_healthTexture.loadFromFile("Art/Effects/Unit_Health.png");
                m_healthTextureLoaded = true;
            }
            float ratio = static_cast<float>(unit->getHealth()) / static_cast<float>(unit->getMaxHealth());
            int frame = static_cast<int>((1.0f - ratio) * 13.0f);
            if (frame > 12) frame = 12;
            if (frame < 0) frame = 0;
            sf::Sprite healthSprite(m_healthTexture);
            healthSprite.setTextureRect(sf::IntRect({ frame * 32, 0 }, { 32, 32 }));
            healthSprite.setPosition(unit->getPosition());
            target.draw(healthSprite);
        }
    }

    for (const auto& e : m_effects) target.draw(e.sprite);
}

void MapManager::drawUI() {
    if (selectionController) selectionController->drawGui();
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

void MapManager::spawnExplosionEffect(sf::Vector2f position, std::shared_ptr<Unit> unit) {
    const AnimationSet* set = AnimationManager::getSet("explosion");
    if (!set) return;
    if (!m_explosionLoaded) {
        if (!m_explosionTexture.loadFromFile("Art/Effects/Explosion.png"))
            return;
        m_explosionLoaded = true;
    }
    Effect e{ sf::Sprite(m_explosionTexture) };
    // Remove unit already
    units.erase(std::remove(units.begin(), units.end(), unit), units.end());

    e.animSet = set;
    e.animState.play("explode", *set);
	SoundManager::play("effects", "explosion");
    e.sprite.setTextureRect(e.animState.getCurrentRect());
    e.sprite.setPosition({ position.x, position.y - 32.f});
    m_effects.push_back(std::move(e));
}

void MapManager::spawnHitEffect(sf::Vector2f position) {
    const AnimationSet* set = AnimationManager::getSet("hitEffect");
    if (!set) return;
    if (!m_hitEffectLoaded) {
        if (!m_hitEffectTexture.loadFromFile("Art/Effects/hitEffect.png"))
            return;
        m_hitEffectLoaded = true;
    }
    Effect e{ sf::Sprite(m_hitEffectTexture) };
    e.animSet = set;
    e.animState.play("hit", *set);
    e.sprite.setTextureRect(e.animState.getCurrentRect());
    e.sprite.setPosition(position);
    m_effects.push_back(std::move(e));
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

std::vector<sf::Vector2i> MapManager::getReachableTiles(sf::Vector2i from, int moveRange, Team movingTeam) const {
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
            if (!mapData[next.x + next.y * static_cast<int>(mapWidth)].isWalkable()) continue;
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

std::vector<sf::Vector2i> MapManager::findPath(sf::Vector2i start, sf::Vector2i goal, Team movingTeam) {
    std::vector<sf::Vector2i> path;
    auto isValid = [&](int x, int y) {
        if (x < 0 || x >= static_cast<int>(mapWidth) || y < 0 || y >= static_cast<int>(mapHeight)) return false;
        return mapData[x + y * static_cast<int>(mapWidth)].isWalkable();
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
    selectionController.reset();
    currentMapPath = mapPath;

    if (!loadMap(mapFile.tilesetPath, mapFile.tileSize, mapFile.tiles, mapFile.width, mapFile.height))
        return false;

    for (const auto& spawn : mapFile.spawns) {
        auto unit = UnitRegistry::create(spawn.typeName, spawn.team);
        if (unit) spawnUnit(unit, spawn.gridX, spawn.gridY);
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
    return state;
}

bool MapManager::restoreGameState(const std::string& savePath) {
    GameState state;
    if (!FileManager::loadGame(savePath, state)) return false;

    MapFile mapFile;
    if (!FileManager::loadMap(state.mapFilePath, mapFile)) return false;

    units.clear();
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
    return true;
}
