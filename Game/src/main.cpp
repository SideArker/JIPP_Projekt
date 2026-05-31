#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include <optional>
#include "MapManager.hpp"
#include "Unit.hpp"
#include "FileManager.hpp"
#include "MapFile.hpp"
#include "UnitRegistry.hpp"
#include "GameContent.hpp"
#include "TextureManager.hpp"
#include "SoundManager.hpp"


static constexpr const char* LEVEL1_PATH = "levels/level1.map";

static void createDefaultLevel1() {
    MapFile map;
    map.tilesetPath = "Art/Map/map.png";
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
    map.spawns = { { "MissileTank", 5, 5, Team::Ally },{ "Tank", 3, 3, Team::Ally },{ "Soldier" , 3, 4, Team::Ally }, {"Tank", 5, 4, Team::Enemy }};
    FileManager::saveMap(map, LEVEL1_PATH);
}

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "Map Renderer", sf::State::Windowed);
    sf::View view(sf::FloatRect({ 0.f, 0.f }, { 400.f, 300.f }));
    window.setView(view);

    TextureManager textures;
    GameContent::init();

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
        SoundManager::registerMusic("AllyTheme", "Art/Sound/AllyTheme.wav");
		SoundManager::playMusic("AllyTheme");
		SoundManager::setMusicVolume(20.f);


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
