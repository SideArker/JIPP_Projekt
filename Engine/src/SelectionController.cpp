#include "SelectionController.hpp"
#include "MapManager.hpp"
#include "Unit.hpp"
#include <algorithm>
#include <cmath>

SelectionController::SelectionController(sf::RenderWindow& window, MapManager& mapManager)
    : gui(window), mapManager(mapManager), selectedUnit(nullptr) {
    try {
		if (!m_walkOverlayTexture.loadFromFile("Art/Map_Walk_Overlay.png")) {
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

            btn->onMouseEnter([this, &mapManager,  gridPos, tileSize]() {
                if (!selectedUnit) return;
                bool isReachable = std::find(reachableTiles.begin(), reachableTiles.end(), gridPos) != reachableTiles.end();
                if (!isReachable) {
                    previewPath.clear();
                    return;
                }
                sf::Vector2i unitGrid(
                    static_cast<int>(std::round(selectedUnit->getPosition().x / static_cast<float>(tileSize.x))),
                    static_cast<int>(std::round(selectedUnit->getPosition().y / static_cast<float>(tileSize.y)))
                );
                previewPath = mapManager.findPath(unitGrid, gridPos);
                
            });

            btn->onMouseLeave([this]() {
                previewPath.clear();
            });

            btn->onClick([this, &mapManager, gridPos, tileSize]() {
                auto unitAtTile = mapManager.getUnitAtTile(gridPos);
                if (unitAtTile) {
                    selectedUnit = unitAtTile;
                    sf::Vector2i unitGrid(
                        static_cast<int>(std::round(unitAtTile->getPosition().x / static_cast<float>(tileSize.x))),
                        static_cast<int>(std::round(unitAtTile->getPosition().y / static_cast<float>(tileSize.y)))
                    );
                    reachableTiles = mapManager.getReachableTiles(unitGrid, unitAtTile->getMoveSpeed());
                    previewPath.clear();
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
}

void SelectionController::drawGui() {
    gui.draw();
}
