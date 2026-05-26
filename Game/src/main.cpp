#include <SFML/Graphics.hpp>
#include <optional>
#include <memory>
#include "MapManager.hpp"
#include "Unit.hpp"
#include "AnimationManager.hpp"
#include "TextureManager.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "Map Renderer", sf::State::Windowed);
    sf::View view(sf::FloatRect({ 0.f, 0.f }, { 800.f, 500.f }));
    window.setView(view);

    std::vector<Tile> level = {
        // Row 0: Open water
        {7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{7, false},{7, false},{5, false},{5, false},{5, false},{5, false},{5, false},{5, false},{7, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{7, false},{5, false},{1, true}, {1, true}, {2, true}, {1, true}, {1, true}, {1, true}, {6, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{7, false},{1, true}, {1, true}, {1, true}, {1, true}, {3, false},{1, true}, {1, true}, {6, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{1, true}, {1, true}, {1, true}, {2, true}, {1, true}, {1, true}, {1, true}, {1, true}, {6, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{1, true}, {1, true}, {3, false},{1, true}, {1, true}, {1, true}, {1, true}, {1, true}, {1, true}, {6, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{1, true}, {1, true}, {1, true}, {1, true}, {2, true}, {1, true}, {1, true}, {1, true}, {1, true}, {6, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{1, true}, {2, true}, {1, true}, {1, true}, {1, true}, {1, true}, {3, false},{1, true}, {6, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{7, false},{1, true}, {1, true}, {1, true}, {1, true}, {1, true}, {1, true}, {1, true}, {6, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{7, false},{7, false},{1, true}, {1, true}, {2, true}, {1, true}, {1, true}, {6, false},{7, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{7, false},{7, false},{4, false},{4, false},{4, false},{4, false},{4, false},{4, false},{7, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},
        {7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false},{7, false}
    };
    MapManager mapManager;

    if (!mapManager.loadMap("Art/map.png", sf::Vector2u(32, 32), level, 16, 16)) {
        return -1;
    }

    AnimationSet tankAnimSet;
    tankAnimSet
        .addClip("move_left",  AnimationClip{ { sf::IntRect({0,  0}, {32, 32}) }, 0.1f, true, false })
        .addClip("move_right", AnimationClip{ { sf::IntRect({0,  0}, {32, 32}) }, 0.1f, true, true  })
        .addClip("move_down",  AnimationClip{ { sf::IntRect({0, 32}, {32, 32}) }, 0.1f, true, false })
        .addClip("move_up",    AnimationClip{ { sf::IntRect({0, 64}, {32, 32}) }, 0.1f, true, false });
    AnimationManager::registerSet("Tank", std::move(tankAnimSet));

    sf::Color teamColor(50, 255, 50);
    const sf::Texture& tankTex = TextureManager::getTexture("Art/tank-shoot-grayscale.png", "Art/tank-shoot-grayscale-mask.png", teamColor);
    const AnimationSet& tankAnim = *AnimationManager::getSet("Tank");

    auto player = std::make_shared<Unit>("Tank", tankTex, tankAnim, 20, 5, 5);
    mapManager.spawnUnit(player, 5, 5);
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

    TextureManager::clearCache();
    return 0;
}
