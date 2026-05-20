#include "MapManager.hpp"
#include "SelectionController.hpp"
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
    units.push_back(unit);
}

void MapManager::setupInput(sf::RenderWindow& window) {
    selectionController = std::make_unique<SelectionController>(window, *this);
}

void MapManager::handleEvent(const sf::Event& event) {
    if (selectionController) selectionController->handleEvent(event);
}

void MapManager::update(float deltaTime) {
    for (auto& unit : units) {
        unit->update(deltaTime);
    }
}

void MapManager::draw(sf::RenderTarget& target) {
    target.draw(renderer);
    if (selectionController) selectionController->drawOverlays(target);
    for (const auto& unit : units) {
        const std::string& imagePath = unit->getImagePath();
        if (textureCache.find(imagePath) == textureCache.end()) {
            textureCache[imagePath].loadFromFile(imagePath);
        }
        sf::Sprite unitSprite(textureCache[imagePath]);

        // First column (x=0), row determined by direction, each frame is 32x32
        int row = 0;
        bool flipX = false;
        switch (unit->getMoveDirection()) {
            case MoveDirection::Left:  row = 0; break;
            case MoveDirection::Right: row = 0; flipX = true; break;
            case MoveDirection::Down:  row = 1; break;
            case MoveDirection::Up:    row = 2; break;
        }
        unitSprite.setTextureRect(sf::IntRect({ 0, row * 32 }, { 32, 32 }));

        if (flipX) {
            unitSprite.setScale({ -1.f, 1.f });
            unitSprite.setPosition({ unit->getPosition().x + 32.f, unit->getPosition().y });
        } else {
            unitSprite.setPosition(unit->getPosition());
        }

        target.draw(unitSprite);
    }
}

void MapManager::drawUI() {
    if (selectionController) selectionController->drawGui();
}

sf::Vector2u MapManager::getTileSize() const { return tileSize; }
unsigned int MapManager::getMapWidth() const { return mapWidth; }
unsigned int MapManager::getMapHeight() const { return mapHeight; }

std::shared_ptr<Unit> MapManager::getUnitAtTile(sf::Vector2i gridPos) const {
    for (const auto& unit : units) {
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

std::vector<sf::Vector2i> MapManager::getReachableTiles(sf::Vector2i from, int moveRange) const {
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
            reachable.push_back(pos);
        }

        if (steps >= moveRange) continue;

        for (const auto& dir : directions) {
            sf::Vector2i next = pos + dir;
            if (next.x < 0 || next.y < 0 ||
                next.x >= static_cast<int>(mapWidth) ||
                next.y >= static_cast<int>(mapHeight)) continue;
            if (!mapData[next.x + next.y * static_cast<int>(mapWidth)].isWalkable()) continue;
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

std::vector<sf::Vector2i> MapManager::findPath(sf::Vector2i start, sf::Vector2i goal) {
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
