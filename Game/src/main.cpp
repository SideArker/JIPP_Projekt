#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <optional>
#include <memory>
#include <algorithm>
#include <cmath>
#include "MapManager.hpp"
#include "Unit.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "Map Renderer", sf::State::Windowed);
    sf::View view(sf::FloatRect({ 0.f, 0.f }, { 320.f, 180.f }));
    window.setView(view);

    tgui::Gui gui{ window };

    std::vector<Tile> level = {
        {1, true}, {2, true}, {1, true}, {2, true}, {5, false}, {1, true},
        {1, true}, {1, true}, {5, false}, {1, true}, {1, true}, {1, true},
        {1, true}, {5, false}, {1, true}, {1, true}, {1, true}, {1, true},
        {1, true}, {2, true}, {1, true}, {1, true}, {5, false}, {1, true}
    };

    MapManager mapManager;

    if (!mapManager.loadMap("Art/tileset.png", sf::Vector2u(32, 32), level, 6, 4)) {
        return -1;
    }

    auto player = std::make_shared<Unit>("Tank", "Art/Unit-Base.png", 20, 5, 5);
    mapManager.spawnUnit(player, 1, 1);

    const sf::Vector2u tileSize = mapManager.getTileSize();
    const unsigned int mapW = mapManager.getMapWidth();
    const unsigned int mapH = mapManager.getMapHeight();

    const float scaleX = static_cast<float>(window.getSize().x) / view.getSize().x;
    const float scaleY = static_cast<float>(window.getSize().y) / view.getSize().y;

    std::shared_ptr<Unit> selectedUnit = nullptr;
    std::vector<sf::Vector2i> reachableTiles;
    std::vector<sf::Vector2i> previewPath;

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

            btn->onMouseEnter([&selectedUnit, &mapManager, &previewPath, &reachableTiles, gridPos, tileSize]() {
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

            btn->onMouseLeave([&previewPath]() {
                previewPath.clear();
            });

            btn->onClick([&selectedUnit, &mapManager, &reachableTiles, &previewPath, gridPos, tileSize]() {
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

    sf::Clock clock;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            gui.handleEvent(*event);
        }

        float deltaTime = clock.restart().asSeconds();
        mapManager.update(deltaTime);

        window.clear();
        window.setView(view);
        mapManager.draw(window);
        mapManager.drawOverlays(window, reachableTiles, previewPath);
        gui.draw();
        window.display();
    }

    return 0;
}
