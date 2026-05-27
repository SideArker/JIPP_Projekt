#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include <optional>
#include "MapManager.hpp"
#include "Unit.hpp"
#include "AnimationManager.hpp"
#include "TextureManager.hpp"
#include "FileManager.hpp"
#include "MapFile.hpp"
#include "UnitRegistry.hpp"

static constexpr const char* LEVEL1_PATH = "levels/level1.map";

static void createDefaultLevel1() {
    MapFile map;
    map.tilesetPath = "Art/map.png";
    map.tileSize    = { 32, 32 };
    map.width       = 8;
    map.height      = 7;
    map.tiles = {
        {18, true},  {19, true},  {19, true},  {19, true},  {19, true},  {19, true},  {19, true},  {9, true},
        {13, false}, {2, true},   {3, true},   {17, false}, {7, false},  {17, false}, {3, true},   {5, false},
        {13, false}, {2, true},   {7, false},  {7, false},  {17, false}, {7, false},  {7, false},  {5, false},
        {13, false}, {2, true},   {3, true},   {12, true},  {2, true},   {2, true},   {4, true},   {5, false},
        {13, false}, {10, true},  {10, true},  {11, true},  {10, true},  {10, true},  {16, true},  {5, false},
        {13, false}, {2, true},   {2, true},   {2, true},   {2, true},   {2, true},   {2, true},   {5, false},
        {14, false}, {15, false}, {15, false}, {15, false}, {15, false}, {15, false}, {15, false}, {8, false}
    };
    map.spawns = { { "Tank", 5, 5, Team::Ally }, {"Enemy_Tank", 5, 4, Team::Enemy }};
    FileManager::saveMap(map, LEVEL1_PATH);
}

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "Map Renderer", sf::State::Windowed);
    sf::View view(sf::FloatRect({ 0.f, 0.f }, { 800.f, 500.f }));
    window.setView(view);

    AnimationSet tankAnimSet;
    tankAnimSet
        .addClip("move_left",  AnimationClip{ { sf::IntRect({0,  0}, {32, 32}) }, 0.1f, true, false })
        .addClip("move_right", AnimationClip{ { sf::IntRect({0,  0}, {32, 32}) }, 0.1f, true, true  })
        .addClip("move_down",  AnimationClip{ { sf::IntRect({0, 32}, {32, 32}) }, 0.1f, true, false })
        .addClip("move_up",    AnimationClip{ { sf::IntRect({0, 64}, {32, 32}) }, 0.1f, true, false });
    AnimationManager::registerSet("Tank", std::move(tankAnimSet));

    sf::Color teamColor(50, 255, 50);
    sf::Color enemyColor(255, 7, 58);
    const sf::Texture& tankTex  = TextureManager::getTexture("Art/tank-shoot-grayscale.png", "Art/tank-shoot-grayscale-mask.png", teamColor);
    const sf::Texture& enemyTankTex = TextureManager::getTexture("Art/tank-shoot-grayscale.png", "Art/tank-shoot-grayscale-mask.png", enemyColor);

    const AnimationSet& tankAnim = *AnimationManager::getSet("Tank");

    UnitRegistry::registerType("Tank", [&](Team team) {
        auto unit = std::make_shared<Unit>("Tank", tankTex, tankAnim, 20, 5, 5);
        unit->setTeam(team);
        return unit;
    });

    UnitRegistry::registerType("Enemy_Tank", [&](Team team) {
        auto unit = std::make_shared<Unit>("Enemy_Tank", enemyTankTex, tankAnim, 20, 5, 5);
        unit->setTeam(team);
        return unit;
        });

    if (!std::filesystem::exists(LEVEL1_PATH))
        createDefaultLevel1();

    MapManager mapManager;
    if (!mapManager.loadFromFile(LEVEL1_PATH)) return -1;
    mapManager.setupInput(window);

    sf::Clock clock;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
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
