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
#include <SFML/Graphics.hpp>
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
  tgui::ToolTip::setInitialDelay(std::chrono::milliseconds(0));

  createDefaultLevel1();
  MapManager mapManager;
  GameContent::configure(mapManager);
  if (!mapManager.loadFromFile(LEVEL1_PATH))
    return -1;
  mapManager.setupInput(window);

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

    // Populate team list
    panels.teamList->removeAllWidgets();
    int teamY = 0;
    for (const auto &[team, data] : mapManager.getTeams()) {
      if (team == Team::Neutral) continue;
      auto nameLbl = tgui::Label::create();
      nameLbl->setText(data.name);
      nameLbl->getRenderer()->setTextColor(data.color);
      nameLbl->setTextSize(14);
      nameLbl->setPosition(5, teamY);
      panels.teamList->add(nameLbl);

      auto moneyLbl = tgui::Label::create();
      moneyLbl->setText("$" + std::to_string(data.startMoney));
      moneyLbl->getRenderer()->setTextColor(sf::Color(220, 220, 220)); // slightly distinct color for money
      moneyLbl->setTextSize(14);
      moneyLbl->setPosition("100% - 65", teamY);
      panels.teamList->add(moneyLbl);
      teamY += 25;
    }

    // Hook up selection callback
    mapManager.setOnSelectionChanged(
        [infoLabel = panels.infoLabel, flagsList = panels.flagsList](
            std::shared_ptr<Unit> unit, std::shared_ptr<Building> building,
            const Tile *tile) {
          flagsList->removeAllWidgets();
          if (unit) {
            std::string desc = unit->getName() + "\n";
            desc += "Type: Unit\n";
            desc += "HP: " + std::to_string(unit->getHealth()) + "/" +
                    std::to_string(unit->getMaxHealth()) + "\n";
            desc += "Attack: " + std::to_string(unit->getDamage());
            infoLabel->setText(desc);

            int flagY = 10;
            if (unit->hasFlag(UnitFlag::Capture)) {
              tgui::Texture tex("Art/UI/flag.png", tgui::UIntRect(0, 0, 32, 32));
              auto pic = tgui::Picture::create(tex);
              pic->setPosition(10, flagY);
              auto tooltip = tgui::Label::create("Can capture buildings");
              tooltip->getRenderer()->setBackgroundColor(
                  sf::Color(40, 40, 45, 230));
              tooltip->getRenderer()->setTextColor(sf::Color::White);
              tooltip->setTextSize(14);
              pic->setToolTip(tooltip);
              flagsList->add(pic);
              flagY += 40;
            }
          } else if (building) {
            infoLabel->setText(building->getTypeName() +
                               "\n\nHP: --/--\nAttack: --");
          } else {
            infoLabel->setText("");
          }
        });
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
