#include "SelectionController.hpp"
#include "MapManager.hpp"
#include "Unit.hpp"
#include <algorithm>
#include <cmath>

static sf::Vector2i computeApproachDir(sf::Vector2f localPos, float btnW, float btnH) {
    float rx = localPos.x / btnW;
    float ry = localPos.y / btnH;

    // Centre deadzone: show the easiest (shortest) attack path
    const float deadzone = 0.28f;
    if (rx > deadzone && rx < 1.f - deadzone && ry > deadzone && ry < 1.f - deadzone)
        return {0, 0};

    // Outer area divided by diagonal lines into 4 quadrants.
    // Corners are assigned to the nearest cardinal direction.
    if (ry < rx && ry < 1.f - rx) return { 0, -1 };  // top
    if (ry > rx && ry > 1.f - rx) return { 0,  1 };  // bottom
    if (rx <= ry && rx <= 1.f - ry) return {-1,  0 }; // left
    return { 1,  0 };                                   // right
}

SelectionController::SelectionController(sf::RenderWindow& window, MapManager& mapManager, const std::string& walkOverlayPath)
    : gui(window), mapManager(mapManager), selectedUnit(nullptr) {
    try {
		if (!m_walkOverlayTexture.loadFromFile(walkOverlayPath)) {
			throw std::runtime_error("Failed to load walk overlay texture");
		}
	}
    catch (const std::exception& e) {
        std::cerr << "Error loading walk overlay texture: " << e.what() << std::endl;
    }
    const sf::Vector2u tileSize = mapManager.getTileSize();
    const unsigned int mapW = mapManager.getMapWidth();
    const unsigned int mapH = mapManager.getMapHeight();
    const sf::View view = window.getView();
    const float scaleX = static_cast<float>(window.getSize().x) / view.getSize().x;
    const float scaleY = static_cast<float>(window.getSize().y) / view.getSize().y;
    m_scaleX = scaleX;
    m_scaleY = scaleY;

    for (unsigned int ty = 0; ty < mapH; ++ty) {
        for (unsigned int tx = 0; tx < mapW; ++tx) {
            auto btn = tgui::Button::create();
            btn->setPosition(tx * tileSize.x * scaleX, ty * tileSize.y * scaleY);
            btn->setSize(tileSize.x * scaleX, tileSize.y * scaleY);
            btn->setText("");
            btn->getRenderer()->setBackgroundColor(sf::Color::Transparent);
            btn->getRenderer()->setBackgroundColorHover(sf::Color::Transparent);
            btn->getRenderer()->setBackgroundColorDown(sf::Color::Transparent);
            btn->getRenderer()->setBorderColor(sf::Color::Transparent);
            btn->getRenderer()->setBorderColorHover(sf::Color::Transparent);
            btn->getRenderer()->setBorderColorDown(sf::Color::Transparent);
            btn->getRenderer()->setTextColor(sf::Color::Transparent);
            btn->getRenderer()->setBorders(tgui::Borders(0));

            const sf::Vector2i gridPos(tx, ty);

            btn->onMouseEnter([this, &mapManager, gridPos, tileSize]() {
                if (mapManager.isAnyUnitActing() || !selectedUnit) return;

                sf::Vector2i unitGrid(
                    static_cast<int>(std::round(selectedUnit->getPosition().x / static_cast<float>(tileSize.x))),
                    static_cast<int>(std::round(selectedUnit->getPosition().y / static_cast<float>(tileSize.y)))
                );

                // Check for enemy unit on this tile
                auto unitAtTile = mapManager.getUnitAtTile(gridPos);
                if (unitAtTile && unitAtTile != selectedUnit && unitAtTile->getTeam() != selectedUnit->getTeam()) {
                    hoveredEnemyUnit = unitAtTile;
                    m_preferredApproachDir = {0, 0};
                    updateAttackPath(gridPos, unitGrid, {0, 0});
                    return;
                }

                hoveredEnemyUnit = nullptr;

                bool isReachable = std::find(reachableTiles.begin(), reachableTiles.end(), gridPos) != reachableTiles.end();
                if (!isReachable) {
                    previewPath.clear();
                    return;
                }
                previewPath = mapManager.findPath(unitGrid, gridPos, selectedUnit->getTeam());
            });

            btn->onMouseLeave([this]() {
                previewPath.clear();
                hoveredEnemyUnit = nullptr;
                m_preferredApproachDir = {0, 0};
            });

            btn->onClick([this, &mapManager, gridPos, tileSize]() {
                if (mapManager.isAnyUnitActing()) return;

                if (hoveredEnemyUnit && selectedUnit) {
                    sf::Vector2i attackerGrid(
                        static_cast<int>(std::round(selectedUnit->getPosition().x / static_cast<float>(tileSize.x))),
                        static_cast<int>(std::round(selectedUnit->getPosition().y / static_cast<float>(tileSize.y)))
                    );
                    sf::Vector2i attackerEnd = previewPath.empty() ? attackerGrid : previewPath.back();
                    int dx = gridPos.x - attackerEnd.x;
                    int dy = gridPos.y - attackerEnd.y;
                    MoveDirection shootDir = (std::abs(dx) >= std::abs(dy))
                        ? (dx >= 0 ? MoveDirection::Right : MoveDirection::Left)
                        : (dy >= 0 ? MoveDirection::Down : MoveDirection::Up);

                    if (!previewPath.empty()) {
                        selectedUnit->move(previewPath);
                    }
                    selectedUnit->dealDamage(hoveredEnemyUnit, shootDir);
                    selectedUnit = nullptr;
                    reachableTiles.clear();
                    previewPath.clear();
                    hoveredEnemyUnit = nullptr;
                    return;
                }

                auto unitAtTile = mapManager.getUnitAtTile(gridPos);
                if (unitAtTile) {
                    if (unitAtTile->getTeam() == Team::Enemy) return;
                    selectedUnit = unitAtTile;
                    sf::Vector2i unitGrid(
                        static_cast<int>(std::round(unitAtTile->getPosition().x / static_cast<float>(tileSize.x))),
                        static_cast<int>(std::round(unitAtTile->getPosition().y / static_cast<float>(tileSize.y)))
                    );
                    reachableTiles = mapManager.getReachableTiles(unitGrid, unitAtTile->getMoveSpeed(), unitAtTile->getTeam());
                    previewPath.clear();
                    hoveredEnemyUnit = nullptr;
                    return;
                }

                if (!selectedUnit) return;

                bool isReachable = std::find(reachableTiles.begin(), reachableTiles.end(), gridPos) != reachableTiles.end();
                if (isReachable && !previewPath.empty()) {
                    selectedUnit->move(previewPath);
                }
                selectedUnit = nullptr;
                reachableTiles.clear();
                previewPath.clear();
            });

            gui.add(btn);
        }
    }
}

void SelectionController::handleEvent(const sf::Event& event) {
    if (auto* mm = event.getIf<sf::Event::MouseMoved>()) {
        if (hoveredEnemyUnit && selectedUnit && !mapManager.isAnyUnitActing()) {
            const sf::Vector2u tileSize = mapManager.getTileSize();
            float scaledW = tileSize.x * m_scaleX;
            float scaledH = tileSize.y * m_scaleY;

            sf::Vector2i enemyGrid(
                static_cast<int>(std::round(hoveredEnemyUnit->getPosition().x / static_cast<float>(tileSize.x))),
                static_cast<int>(std::round(hoveredEnemyUnit->getPosition().y / static_cast<float>(tileSize.y)))
            );

            int tileX = static_cast<int>(mm->position.x / scaledW);
            int tileY = static_cast<int>(mm->position.y / scaledH);

            if (tileX == enemyGrid.x && tileY == enemyGrid.y) {
                float localX = mm->position.x - tileX * scaledW;
                float localY = mm->position.y - tileY * scaledH;

                sf::Vector2i newDir = computeApproachDir({localX, localY}, scaledW, scaledH);
                if (newDir != m_preferredApproachDir) {
                    m_preferredApproachDir = newDir;
                    sf::Vector2i unitGrid(
                        static_cast<int>(std::round(selectedUnit->getPosition().x / static_cast<float>(tileSize.x))),
                        static_cast<int>(std::round(selectedUnit->getPosition().y / static_cast<float>(tileSize.y)))
                    );
                    updateAttackPath(enemyGrid, unitGrid, m_preferredApproachDir);
                }
            }
        }
    }
    gui.handleEvent(event);
}

void SelectionController::drawOverlays(sf::RenderTarget& target) const {
    const sf::Vector2u tileSize = mapManager.getTileSize();
    sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)));

    sf::Sprite walkSprite(m_walkOverlayTexture);
    for (const auto& pos : reachableTiles) {
        walkSprite.setPosition(sf::Vector2f(pos.x * static_cast<float>(tileSize.x), pos.y * static_cast<float>(tileSize.y)));
        target.draw(walkSprite);
    }

    overlay.setFillColor(sf::Color(100, 255, 100, 160));
    for (const auto& pos : previewPath) {
        overlay.setPosition(sf::Vector2f(pos.x * static_cast<float>(tileSize.x), pos.y * static_cast<float>(tileSize.y)));
        target.draw(overlay);
    }

    // Red highlight on the enemy tile being targeted
    if (hoveredEnemyUnit) {
        sf::Vector2i enemyGrid(
            static_cast<int>(std::round(hoveredEnemyUnit->getPosition().x / static_cast<float>(tileSize.x))),
            static_cast<int>(std::round(hoveredEnemyUnit->getPosition().y / static_cast<float>(tileSize.y)))
        );
        overlay.setFillColor(sf::Color(255, 50, 50, 160));
        overlay.setPosition(sf::Vector2f(enemyGrid.x * static_cast<float>(tileSize.x), enemyGrid.y * static_cast<float>(tileSize.y)));
        target.draw(overlay);
    }
}

void SelectionController::drawGui() {
    gui.draw();
}

void SelectionController::updateAttackPath(sf::Vector2i enemyGrid, sf::Vector2i unitGrid, sf::Vector2i preferredDir) {
    previewPath.clear();

    int minRange = selectedUnit->getMinAttackRange();
    int maxRange = selectedUnit->getMaxAttackRange();

    auto inRange = [&](sf::Vector2i tile) {
        int d = std::max(std::abs(enemyGrid.x - tile.x), std::abs(enemyGrid.y - tile.y));
        return d >= minRange && d <= maxRange;
    };

    // Already within attack range
    if (inRange(unitGrid)) return;

    //  try the requested tile first (mainly useful for melee units)
    if (preferredDir != sf::Vector2i{0, 0}) {
        sf::Vector2i preferred = enemyGrid + preferredDir;
        if (preferred != unitGrid && inRange(preferred) &&
            std::find(reachableTiles.begin(), reachableTiles.end(), preferred) != reachableTiles.end()) {
            auto path = mapManager.findPath(unitGrid, preferred, selectedUnit->getTeam());
            if (!path.empty()) {
                previewPath = path;
                return;
            }
        }
    }

    // Find shortest path to any reachable tile within attack range
    std::vector<sf::Vector2i> bestPath;
    for (const auto& tile : reachableTiles) {
        if (!inRange(tile)) continue;
        auto path = mapManager.findPath(unitGrid, tile, selectedUnit->getTeam());
        if (!path.empty() && (bestPath.empty() || path.size() < bestPath.size())) {
            bestPath = path;
        }
    }
    previewPath = bestPath;
}
