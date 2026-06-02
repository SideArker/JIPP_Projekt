#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include <optional>
#include "CameraController.hpp"
#include "RightPanelUI.hpp"
#include "MapManager.hpp"
#include "Unit.hpp"
#include "TerrainMovement.hpp"
#include "FileManager.hpp"
#include "MapFile.hpp"
#include "UnitRegistry.hpp"
#include "GameContent.hpp"
#include "TextureManager.hpp"
#include "SoundManager.hpp"
#include "AIController.hpp"
#include <iostream>

static constexpr const char* LEVEL1_PATH = "levels/editor.map";

static void createDefaultLevel1() {
    MapFile map;
    map.tilesetPath = "Art/Map/map.png";
    map.tileSize = { 32, 32 };
    map.width = 8;
    map.height = 7;
    using T = TerrainType;
    map.tiles = {
        {18, T::Grass},    {19, T::Grass},    {19, T::Grass},    {19, T::Grass},    {19, T::Grass},    {19, T::Grass},    {19, T::Grass},    {9,  T::Grass},
        {13, T::Grass},    {2,  T::Grass},    {3,  T::Grass},    {17, T::Mountain}, {7,  T::Mountain}, {17, T::Mountain}, {3,  T::Grass},    {5,  T::Grass},
        {13, T::Grass},    {2,  T::Grass},    {7,  T::Mountain}, {7,  T::Mountain}, {17, T::Mountain}, {7,  T::Mountain}, {7,  T::Mountain}, {5,  T::Grass},
        {13, T::Grass},    {2,  T::Grass},    {3,  T::Grass},    {12, T::Road},     {2,  T::Grass},    {2,  T::Grass},    {4,  T::Grass},    {5,  T::Grass},
        {13, T::Grass},    {10, T::Road},     {10, T::Road},     {11, T::Road},     {10, T::Road},     {10, T::Road},     {16, T::Road},     {5,  T::Grass},
        {13, T::Grass},    {2,  T::Grass},    {2,  T::Grass},    {2,  T::Grass},    {2,  T::Grass},    {2,  T::Grass},    {2,  T::Grass},    {5,  T::Grass},
        {14, T::Grass},    {15, T::Grass},    {15, T::Grass},    {15, T::Grass},    {15, T::Grass},    {15, T::Grass},    {15, T::Grass},    {8,  T::Grass}
    };
    map.spawns = { { "MissileTank", 1, 1, Team::Ally },{ "Tank", 3, 3, Team::Ally },{ "Soldier" , 3, 4, Team::Ally }, {"Tank", 5, 4, Team::Enemy }};
    map.buildingSpawns = {
        { "HQ", 1, 5, Team::Ally    },
        { "HQ", 6, 1, Team::Enemy   },
        { "Factory", 2, 5, Team::Ally    },
        { "OilRig",  6, 5, Team::Neutral }
    };
    FileManager::saveMap(map, LEVEL1_PATH);
}

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1920, 1080 }), "Map Renderer", sf::State::Windowed);
    sf::View view(sf::FloatRect({ 0.f, 0.f }, { 480.f, 256.f }));
    view.setViewport(sf::FloatRect({ 0.f, 0.f }, { 1440.f / 1920.f, 768.f / 1080.f }));


    sf::Vector2f center = sf::Vector2f(0.f, 0.f);

    window.setView(view);
    GameContent::init();

    MapManager mapManager;
    GameContent::configure(mapManager);
    if (!mapManager.loadFromFile(LEVEL1_PATH)) return -1;
    mapManager.setupInput(window);

    CameraController cameraController;
    sf::Vector2u mapPixels(
        mapManager.getMapWidth()  * mapManager.getTileSize().x,
        mapManager.getMapHeight() * mapManager.getTileSize().y
    );
    cameraController.init(view, mapPixels, window.getSize());

    if (tgui::Gui* gui = mapManager.getGui()) {
        auto panels = buildRightPanelUI(
            *gui,
            480.f, 0.f,
            static_cast<float>(window.getSize().x) / 640.f,
            static_cast<float>(window.getSize().y) / 360.f,
            2, 80.f
        );
        panels.endTurnBtn->onClick([&mapManager]() { mapManager.requestEndTurn(); });
    }

    AIController aiController;
    sf::Clock clock;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                break;
            }
            mapManager.handleEvent(*event);
        }

        if (!window.isOpen())
            break;

        float deltaTime = clock.restart().asSeconds();

        if (mapManager.getCurrentTeam() == Team::Enemy) {
            if (!mapManager.isAnyUnitActing()) {
                aiController.update(deltaTime, mapManager, mapManager.getTurnController(), cameraController);
            }
            if (aiController.isDone() && !mapManager.isAnyUnitActing()) {
                mapManager.endTurn();
            }
        } else {
            cameraController.release();
            aiController.reset();
        }

        mapManager.update(deltaTime);
        cameraController.update(deltaTime, window);
        mapManager.syncCameraView(cameraController.getView());

        window.clear();
        window.setView(cameraController.getView());
        mapManager.draw(window);
        mapManager.drawUI();
        window.display();
        
    }
    
    SoundManager::shutdown();
    TextureManager::clearCache();
    return 0;
}
