#include <SFML/Graphics.hpp>
#include <optional>
#include "MapRenderer.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "Map Renderer");

    std::vector<int> level = {
        0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 0,
        0, 1, 2, 2, 1, 0,
        0, 1, 1, 1, 1, 0
    };

    MapRenderer map;

    if (!map.load("tileset.png", sf::Vector2u(32, 32), level, 6, 4)) {
        return -1;
    }

    while (window.isOpen()) {
        // SFML 3.x uses std::optional and std::variant for events
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        window.clear();
        window.draw(map);
        window.display();
    }

    return 0;
}