#include "AIController.hpp"
#include "CameraController.hpp"
#include "FileManager.hpp"
#include "GameContent.hpp"
#include "MapFile.hpp"
#include "MapManager.hpp"
#include "RightPanelUI.hpp"
#include "SoundManager.hpp"
#include "TerrainMovement.hpp"
#include "TextureManager.hpp"
#include "Unit.hpp"
#include "UnitRegistry.hpp"
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>

static constexpr const char *LEVEL1_PATH = "levels/editor.map";

static void createDefaultLevel1() {
  MapFile map;
  map.tilesetPath = "Art/Map/map.png";
  map.tileSize = {32, 32};
  map.width = 8;
  map.height = 7;
  using T = TerrainType;
  map.tiles = {
      {18, T::Grass},    {19, T::Grass},    {19, T::Grass},   {19, T::Grass},
      {19, T::Grass},    {19, T::Grass},    {19, T::Grass},   {9, T::Grass},
      {13, T::Grass},    {2, T::Grass},     {3, T::Grass},    {17, T::Mountain},
      {7, T::Mountain},  {17, T::Mountain}, {3, T::Grass},    {5, T::Grass},
      {13, T::Grass},    {2, T::Grass},     {7, T::Mountain}, {7, T::Mountain},
      {17, T::Mountain}, {7, T::Mountain},  {7, T::Mountain}, {5, T::Grass},
      {13, T::Grass},    {2, T::Grass},     {3, T::Grass},    {12, T::Road},
      {2, T::Grass},     {2, T::Grass},     {4, T::Grass},    {5, T::Grass},
      {13, T::Grass},    {10, T::Road},     {10, T::Road},    {11, T::Road},
      {10, T::Road},     {10, T::Road},     {16, T::Road},    {5, T::Grass},
      {13, T::Grass},    {2, T::Grass},     {2, T::Grass},    {2, T::Grass},
      {2, T::Grass},     {2, T::Grass},     {2, T::Grass},    {5, T::Grass},
      {14, T::Grass},    {15, T::Grass},    {15, T::Grass},   {15, T::Grass},
      {15, T::Grass},    {15, T::Grass},    {15, T::Grass},   {8, T::Grass}};
  map.spawns = {{"MissileTank", 1, 1, Team::Ally},
                {"Tank", 3, 3, Team::Ally},
                {"Soldier", 3, 4, Team::Ally},
                {"Tank", 5, 4, Team::Enemy}};
  map.buildingSpawns = {{"HQ", 1, 5, Team::Ally},
                        {"HQ", 6, 1, Team::Enemy},
                        {"Factory", 2, 5, Team::Ally},
                        {"OilRig", 6, 5, Team::Neutral}};
  FileManager::saveMap(map, LEVEL1_PATH);
}

int main() {
  sf::RenderWindow window(sf::VideoMode({1280, 720}), "Map Renderer",
                          sf::State::Windowed);
  sf::View view(sf::FloatRect({0.f, 0.f}, {640.f, 360.f}));

  window.setView(view);
  GameContent::init();
  tgui::Texture::setDefaultSmooth(false);

  createDefaultLevel1();
  MapManager mapManager;
  GameContent::configure(mapManager);
  if (!mapManager.loadFromFile(LEVEL1_PATH))
    return -1;
  mapManager.setupInput(window);

  // Set game view to match the new UI empty space
  // Window is 1280x720. 
  // Right panel is 250px wide, Bottom panel is 150px tall.
  // Remaining space: 1030 x 570.
  // Let's use a logical view size that keeps tiles nicely sized, e.g., 515 x 285
  view.setSize({515.f, 285.f});
  view.setCenter({257.5f, 142.5f});
  view.setViewport(sf::FloatRect({0.f, 0.f}, {1030.f / 1280.f, 570.f / 720.f}));

  CameraController cameraController;
  sf::Vector2u mapPixels(mapManager.getMapWidth() * mapManager.getTileSize().x,
                         mapManager.getMapHeight() *
                             mapManager.getTileSize().y);
  cameraController.init(view, mapPixels, window.getSize());

  if (tgui::Gui *gui = mapManager.getGui()) {
    auto panels = buildGameUI(*gui);
    panels.endTurnBtn->onClick(
        [&mapManager]() { mapManager.requestEndTurn(); });
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
        aiController.update(deltaTime, mapManager,
                            mapManager.getTurnController(), cameraController);
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

    // Draw the overlay (static)
    window.setView(view);
    mapManager.drawUI();
    window.display();
  }

  SoundManager::shutdown();
  TextureManager::clearCache();
  return 0;
}
