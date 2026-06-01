#include "SelectionController.hpp"
#include "MapManager.hpp"
#include "TurnController.hpp"
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

SelectionController::SelectionController(sf::RenderWindow& window, MapManager& mapManager, TurnController& turnController, const std::string& walkOverlayPath, const std::string& moveArrowPath, const std::string& iconsPath, const std::string& enemyOverlayPath)
    : gui(window), mapManager(mapManager), m_turnController(turnController), selectedUnit(nullptr) {
    try {
		if (!m_walkOverlayTexture.loadFromFile(walkOverlayPath)) {
			throw std::runtime_error("Failed to load walk overlay texture");
		}
		if (!m_moveArrowTexture.loadFromFile(moveArrowPath)) {
			throw std::runtime_error("Failed to load move arrow texture");
		}
		if (!m_iconsTexture.loadFromFile(iconsPath)) {
			throw std::runtime_error("Failed to load icons texture");
		}
		if (!enemyOverlayPath.empty() && !m_enemyOverlayTexture.loadFromFile(enemyOverlayPath)) {
			throw std::runtime_error("Failed to load enemy overlay texture");
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
                    const int minRange = selectedUnit->getMinAttackRange();
                    const int maxRange = selectedUnit->getMaxAttackRange();
                    auto canAttackFrom = [&](sf::Vector2i from) {
                        const int dx = std::abs(gridPos.x - from.x);
                        const int dy = std::abs(gridPos.y - from.y);
                        const int distance = (maxRange == 1) ? (dx + dy) : std::max(dx, dy);
                        return distance >= minRange && distance <= maxRange;
                    };

                    bool canAttackThisAction = canAttackFrom(unitGrid);
                    if (!canAttackThisAction) {
                        for (const auto& tile : reachableTiles) {
                            if (canAttackFrom(tile)) {
                                canAttackThisAction = true;
                                break;
                            }
                        }
                    }

                    if (!canAttackThisAction) {
                        hoveredEnemyUnit = nullptr;
                        previewPath.clear();
                        m_preferredApproachDir = {0, 0};
                        m_cursorIconCell = -1;
                        return;
                    }

                    hoveredEnemyUnit = unitAtTile;
                    m_preferredApproachDir = {0, 0};
                    m_cursorIconCell = 1;
                    updateAttackPath(gridPos, unitGrid, {0, 0});
                    return;
                }

                hoveredEnemyUnit = nullptr;

                bool isReachable = std::find(reachableTiles.begin(), reachableTiles.end(), gridPos) != reachableTiles.end();
                if (!isReachable) {
                    previewPath.clear();
                    m_cursorIconCell = -1;
                    return;
                }
                previewPath = mapManager.findPath(unitGrid, gridPos, selectedUnit->getTeam(), selectedUnit->getMovementCategory());
                m_cursorIconCell = 0;
            });

            btn->onMouseLeave([this]() {
                previewPath.clear();
                hoveredEnemyUnit = nullptr;
                m_preferredApproachDir = {0, 0};
                m_cursorIconCell = -1;
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
                    
                    int distanceToTarget = (selectedUnit->getMaxAttackRange() == 1) 
                                            ? (std::abs(dx) + std::abs(dy)) 
                                            : std::max(std::abs(dx), std::abs(dy));

                    if (distanceToTarget < selectedUnit->getMinAttackRange() || distanceToTarget > selectedUnit->getMaxAttackRange()) {
                        // The enemy is completely out of range. 

                        if (!previewPath.empty()) {
                            selectedUnit->move(previewPath);
                            m_turnController.markActed(*selectedUnit);
                        }

                        selectedUnit = nullptr;
                        reachableTiles.clear();
                        previewPath.clear();
                        hoveredEnemyUnit = nullptr;
                        m_cursorIconCell = -1;
                        return;
                    }

                    MoveDirection shootDir = (std::abs(dx) >= std::abs(dy))
                        ? (dx >= 0 ? MoveDirection::Right : MoveDirection::Left)
                        : (dy >= 0 ? MoveDirection::Down : MoveDirection::Up);

                    // Ranged units cannot move and attack in the same action.
                    if (!previewPath.empty() && selectedUnit->getMaxAttackRange() > 1) {
                        selectedUnit->move(previewPath);
                        m_turnController.markActed(*selectedUnit);
                        selectedUnit = nullptr;
                        reachableTiles.clear();
                        previewPath.clear();
                        hoveredEnemyUnit = nullptr;
                        m_cursorIconCell = -1;
                        return;
                    }

                    if (!previewPath.empty()) {
                        selectedUnit->move(previewPath);
                    }

                    auto unitToMark = selectedUnit; // Capture the smart pointer safely
                    auto* self = this;
                    unitToMark->onAttackFinished = [self, unitToMark]() {
                        self->mapManager.runWhenAllActionsFinished([self, unitToMark]() {
                            self->m_turnController.markActed(*unitToMark);
                        });
                    };

                    selectedUnit->dealDamage(hoveredEnemyUnit, shootDir);
                    
                    selectedUnit = nullptr;
                    reachableTiles.clear();
                    previewPath.clear();
                    hoveredEnemyUnit = nullptr;
                    m_cursorIconCell = -1;
                    return;
                }

                auto unitAtTile = mapManager.getUnitAtTile(gridPos);
                if (unitAtTile) {
                    if (unitAtTile->getTeam() == Team::Enemy) return;
                    if (!m_turnController.canAct(*unitAtTile)) return;
                    selectedUnit = unitAtTile;
                    sf::Vector2i unitGrid(
                        static_cast<int>(std::round(unitAtTile->getPosition().x / static_cast<float>(tileSize.x))),
                        static_cast<int>(std::round(unitAtTile->getPosition().y / static_cast<float>(tileSize.y)))
                    );
                    reachableTiles = mapManager.getReachableTiles(unitGrid, unitAtTile->getMoveSpeed(), unitAtTile->getTeam(), unitAtTile->getMovementCategory());
                    previewPath.clear();
                    hoveredEnemyUnit = nullptr;
                    return;
                }

                if (!selectedUnit) {
                    auto building = mapManager.getBuildingAtTile(gridPos);
                    if (building) building->onClicked();
                    return;
                }

                bool isReachable = std::find(reachableTiles.begin(), reachableTiles.end(), gridPos) != reachableTiles.end();
                if (isReachable && !previewPath.empty()) {
                    selectedUnit->move(previewPath);
                }
                m_turnController.markActed(*selectedUnit);
                selectedUnit = nullptr;
                reachableTiles.clear();
                previewPath.clear();
                m_cursorIconCell = -1;
            });

            gui.add(btn);
        }
    }

    auto endTurnPanel = tgui::Panel::create();
    endTurnPanel->setPosition("100% - 150px", "100% - 70px");
    endTurnPanel->setSize(140, 60);
    endTurnPanel->getRenderer()->setBackgroundColor(sf::Color(20, 20, 20, 210));

    m_turnLabel = tgui::Label::create("Turn 1 - Ally");
    m_turnLabel->setPosition(10, 6);
    m_turnLabel->setTextSize(13);
    endTurnPanel->add(m_turnLabel);

    auto endTurnBtn = tgui::Button::create("End Turn");
    endTurnBtn->setPosition(10, 30);
    endTurnBtn->setSize(120, 24);
    endTurnBtn->onClick([this, &mapManager]() {
        if (m_turnController.getCurrentTeam() == Team::Ally && !mapManager.isAnyUnitActing()) {
            selectedUnit = nullptr;
            reachableTiles.clear();
            previewPath.clear();
            hoveredEnemyUnit = nullptr;
            m_cursorIconCell = -1;
            mapManager.endTurn();
        }
    });
    endTurnPanel->add(endTurnBtn);
    gui.add(endTurnPanel);
}

void SelectionController::handleEvent(const sf::Event& event) {
    if (auto* mm = event.getIf<sf::Event::MouseMoved>()) {
        m_cursorPos = sf::Vector2f(static_cast<float>(mm->position.x), static_cast<float>(mm->position.y));

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

    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if ((kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::Space)
            && m_turnController.getCurrentTeam() == Team::Ally
            && !mapManager.isAnyUnitActing()) {
            selectedUnit = nullptr;
            reachableTiles.clear();
            previewPath.clear();
            hoveredEnemyUnit = nullptr;
            m_cursorIconCell = -1;
            mapManager.endTurn();
        }
    }
}

void SelectionController::drawOverlays(sf::RenderTarget& target) {
    const sf::Vector2u tileSize = mapManager.getTileSize();
    const float tw = static_cast<float>(tileSize.x);
    const float th = static_cast<float>(tileSize.y);

    // Compute tiles that should receive the enemy highlight (3x3 box around each in-range enemy)
    std::vector<sf::Vector2i> enemyOverlayTiles;
    if (selectedUnit) {
        const sf::Vector2i unitGrid(
            static_cast<int>(std::round(selectedUnit->getPosition().x / tw)),
            static_cast<int>(std::round(selectedUnit->getPosition().y / th))
        );
        const int minRange = selectedUnit->getMinAttackRange();
        const int maxRange = selectedUnit->getMaxAttackRange();
        const int mapW = static_cast<int>(mapManager.getMapWidth());
        const int mapH = static_cast<int>(mapManager.getMapHeight());

        for (const auto& unit : mapManager.getUnits()) {
            if (unit->getTeam() == selectedUnit->getTeam() || unit->isDead()) continue;

            const sf::Vector2i enemyGrid(
                static_cast<int>(std::round(unit->getPosition().x / tw)),
                static_cast<int>(std::round(unit->getPosition().y / th))
            );

            auto inRange = [&](sf::Vector2i from) {
                int dx = std::abs(enemyGrid.x - from.x);
                int dy = std::abs(enemyGrid.y - from.y);
                int d = (maxRange == 1) ? (dx + dy) : std::max(dx, dy);
                return d >= minRange && d <= maxRange;
            };

            bool reachable = inRange(unitGrid);
            if (!reachable) {
                for (const auto& tile : reachableTiles) {
                    if (inRange(tile)) { reachable = true; break; }
                }
            }
            if (!reachable) continue;

            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    sf::Vector2i boxTile(enemyGrid.x + dx, enemyGrid.y + dy);
                    if (boxTile.x >= 0 && boxTile.y >= 0 && boxTile.x < mapW && boxTile.y < mapH) {
                        if (std::find(enemyOverlayTiles.begin(), enemyOverlayTiles.end(), boxTile) == enemyOverlayTiles.end())
                            enemyOverlayTiles.push_back(boxTile);
                    }
                }
            }
        }
    }

    sf::Sprite walkSprite(m_walkOverlayTexture);
    for (const auto& pos : reachableTiles) {
        if (std::find(enemyOverlayTiles.begin(), enemyOverlayTiles.end(), pos) != enemyOverlayTiles.end()) continue;
        walkSprite.setPosition(sf::Vector2f(pos.x * tw, pos.y * th));
        target.draw(walkSprite);
    }

    if (!enemyOverlayTiles.empty()) {
        sf::Sprite enemySprite(m_enemyOverlayTexture);
        for (const auto& pos : enemyOverlayTiles) {
            enemySprite.setPosition(sf::Vector2f(pos.x * tw, pos.y * th));
            target.draw(enemySprite);
        }
    }

    if (!previewPath.empty() && selectedUnit) {
        sf::Vector2i unitGrid(
            static_cast<int>(std::round(selectedUnit->getPosition().x / tw)),
            static_cast<int>(std::round(selectedUnit->getPosition().y / th))
        );

        // moveDir angle
        auto dirAngle = [](sf::Vector2i from, sf::Vector2i to) -> float {
            if (to.x > from.x) return 0.f;
            if (to.x < from.x) return 180.f;
            if (to.y > from.y) return 90.f;
            return 270.f;
        };

        // corners
        auto cornerAngle = [](sf::Vector2i inDir, sf::Vector2i outDir) -> float {
            if ((inDir.x > 0 && outDir.y > 0) || (inDir.y < 0 && outDir.x < 0)) return 0.f;   // Right→Down, Up→Left
            if ((inDir.y > 0 && outDir.x < 0) || (inDir.x > 0 && outDir.y < 0)) return 90.f;  // Down→Left, Right→Up — wait, need to reconsider
            if ((inDir.x < 0 && outDir.y < 0) || (inDir.y > 0 && outDir.x > 0)) return 180.f; // Left→Up, Down→Right
            return 270.f;                                                                         // Up→Right, Left→Down
        };

        sf::Sprite arrowSprite(m_moveArrowTexture);
        arrowSprite.setOrigin({ 16.f, 16.f });

        arrowSprite.setTextureRect(sf::IntRect({ 0, 0 }, { 32, 32 }));
        arrowSprite.setRotation(sf::degrees(dirAngle(unitGrid, previewPath[0])));
        arrowSprite.setPosition({ unitGrid.x * tw + tw * 0.5f, unitGrid.y * th + th * 0.5f });
        target.draw(arrowSprite);

        for (int i = 0; i < static_cast<int>(previewPath.size()); ++i) {
            const sf::Vector2i& cur = previewPath[i];
            sf::Vector2i prev = (i == 0) ? unitGrid : previewPath[i - 1];
            bool isLast = (i == static_cast<int>(previewPath.size()) - 1);

            sf::Vector2i inDir = { cur.x - prev.x, cur.y - prev.y };
            float angle;
            int cellX;

            if (isLast) {
                cellX = 64;
                angle = dirAngle(prev, cur);
            } else {
                const sf::Vector2i& next = previewPath[i + 1];
                sf::Vector2i outDir = { next.x - cur.x, next.y - cur.y };
                if (inDir.x != outDir.x || inDir.y != outDir.y) {
                    cellX = 32;
                    angle = cornerAngle(inDir, outDir);
                } else {
                    cellX = 0;
                    angle = dirAngle(prev, cur);
                }
            }

            arrowSprite.setTextureRect(sf::IntRect({ cellX, 0 }, { 32, 32 }));
            arrowSprite.setRotation(sf::degrees(angle));
            arrowSprite.setPosition({ cur.x * tw + tw * 0.5f, cur.y * th + th * 0.5f });
            target.draw(arrowSprite);
        }

        // Ghost of the selected unit at the path destination
        const sf::Vector2i& end = previewPath.back();
        sf::IntRect rect = selectedUnit->getCurrentRect();
        sf::Sprite ghost(selectedUnit->getCurrentTexture());
        ghost.setTextureRect(rect);
        ghost.setColor(sf::Color(255, 255, 255, 150));
        if (selectedUnit->shouldFlipX()) {
            ghost.setScale({ -1.f, 1.f });
            ghost.setPosition({ end.x * tw + static_cast<float>(rect.size.x), end.y * th });
        } else {
            ghost.setPosition({ end.x * tw, end.y * th });
        }
        target.draw(ghost);
    }

    if (hoveredEnemyUnit) {
        sf::RectangleShape overlay(sf::Vector2f(tw, th));
        sf::Vector2i enemyGrid(
            static_cast<int>(std::round(hoveredEnemyUnit->getPosition().x / tw)),
            static_cast<int>(std::round(hoveredEnemyUnit->getPosition().y / th))
        );
        overlay.setFillColor(sf::Color(255, 50, 50, 160));
        overlay.setPosition(sf::Vector2f(enemyGrid.x * tw, enemyGrid.y * th));
        target.draw(overlay);
    }
}

void SelectionController::drawCursorIcon(sf::RenderTarget& target) {
    if (m_cursorIconCell < 0) return;

    sf::View savedView = target.getView();
    target.setView(target.getDefaultView());

    sf::Sprite icon(m_iconsTexture);
    icon.setTextureRect(sf::IntRect({ m_cursorIconCell * 32, 0 }, { 32, 32 }));
    icon.setScale({ 1.2f, 1.2f });
    icon.setPosition({ m_cursorPos.x + 15.f, m_cursorPos.y + 15.f });
    target.draw(icon);

    target.setView(savedView);
}

void SelectionController::drawGui() {
    if (m_turnLabel) {
        const std::string teamStr = (m_turnController.getCurrentTeam() == Team::Ally) ? "Ally" : "Enemy";
        m_turnLabel->setText("Turn " + std::to_string(m_turnController.getTurnNumber()) + " - " + teamStr);
    }
    gui.draw();
}

void SelectionController::updateAttackPath(sf::Vector2i enemyGrid, sf::Vector2i unitGrid, sf::Vector2i preferredDir) {
    previewPath.clear();

    int minRange = selectedUnit->getMinAttackRange();
    int maxRange = selectedUnit->getMaxAttackRange();

    auto inRange = [&](sf::Vector2i tile) {
        int dx = std::abs(enemyGrid.x - tile.x);
        int dy = std::abs(enemyGrid.y - tile.y);
        // Melee units (maxRange == 1) use Manhattan distance - no diagonal attacks.
        // Ranged units use Chebyshev distance.
        int d = (maxRange == 1) ? (dx + dy) : std::max(dx, dy);
        return d >= minRange && d <= maxRange;
    };

    // Already within attack range
    if (inRange(unitGrid)) return;

    //  try the requested tile first (useful for melee units)
    if (preferredDir != sf::Vector2i{0, 0}) {
        sf::Vector2i preferred = enemyGrid + preferredDir;
        if (preferred != unitGrid && inRange(preferred) &&
            std::find(reachableTiles.begin(), reachableTiles.end(), preferred) != reachableTiles.end()) {
            auto path = mapManager.findPath(unitGrid, preferred, selectedUnit->getTeam(), selectedUnit->getMovementCategory());
            if (!path.empty()) {
                previewPath = path;
                return;
            }
        }
    }

    // Find shortest path to tile within attack range
    std::vector<sf::Vector2i> bestPath;
    for (const auto& tile : reachableTiles) {
        if (!inRange(tile)) continue;
        auto path = mapManager.findPath(unitGrid, tile, selectedUnit->getTeam(), selectedUnit->getMovementCategory());
        if (!path.empty() && (bestPath.empty() || path.size() < bestPath.size())) {
            bestPath = path;
        }
    }
    previewPath = bestPath;
}
