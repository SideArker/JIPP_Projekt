#include <SFML/Graphics.hpp>
#include <optional>
#include <memory>
#include "MapManager.hpp"
#include "Unit.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "Map Renderer", sf::State::Windowed);
    sf::View view(sf::FloatRect({ 0.f, 0.f }, { 320.f, 180.f }));
    window.setView(view);

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

    auto player = std::make_shared<Unit>("Tank", "Art/tank-Shoot.png", 20, 5, 5);
    mapManager.spawnUnit(player, 1, 1);
    mapManager.setupInput(window);

    sf::Clock clock;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            mapManager.handleEvent(*event);
        }

        float deltaTime = clock.restart().asSeconds();
        mapManager.update(deltaTime);

        window.clear();
        window.setView(view);
        mapManager.draw(window);
        mapManager.drawUI();
        window.display();
    }

    return 0;
}
