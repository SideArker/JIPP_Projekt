#include "MapEditorApp.hpp"
#include "FileManager.hpp"
#include <algorithm>
#include <filesystem>

MapEditorApp::MapEditorApp()
    : m_window(sf::VideoMode({1600, 900}), "Map Editor",
               sf::Style::Titlebar | sf::Style::Close),
      m_gui(m_window), m_mapView(sf::FloatRect({0.f, 0.f}, {1200.f, 900.f})) {
  m_window.setFramerateLimit(60);
  m_gui.setFont("Art/Fonts/joystixMonospace.ttf");

  TeamRegistry::setColor(Team::Ally, sf::Color(50, 255, 50));
  TeamRegistry::setColor(Team::Enemy, sf::Color(255, 7, 58));
  TeamRegistry::setColor(Team::Neutral, sf::Color(200, 200, 200));

  loadSpritePreviews();

  if (!loadMap(DEFAULT_MAP_PATH)) {
    m_map = createDefaultMap();
    rebuildRenderer();
  }

  m_mapView.setCenter({mapViewportWidth() * 0.5f, mapViewportHeight() * 0.5f});
  clampMapView();

  buildUi();
  refreshStatus("Ready");
}

void MapEditorApp::run() {
  sf::Clock frameClock;
  while (m_window.isOpen()) {
    const float dt = frameClock.restart().asSeconds();
    updateMapView(dt);

    while (const std::optional event = m_window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        m_window.close();
        break;
      }

      if (const auto *resized = event->getIf<sf::Event::Resized>()) {
        m_mapView.setSize(
            {mapViewportWidth() * m_zoom, mapViewportHeight() * m_zoom});
        clampMapView();
        (void)resized;
      }

      if (const auto *wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
        if (wheel->wheel == sf::Mouse::Wheel::Vertical) {
          std::cout << m_zoom;
          if (wheel->delta > 0)
            m_zoom *= 0.85f;
          else
            m_zoom *= 1.15f;
          m_zoom = std::clamp(m_zoom, 0.2f, 5.0f);
          m_mapView.setSize(
              {mapViewportWidth() * m_zoom, mapViewportHeight() * m_zoom});
          clampMapView();
        }
      }

      if (const auto *kp = event->getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::R) {
          m_currentRotation = (m_currentRotation + 1) % 4;
          if (m_rotationLabel) {
            m_rotationLabel->setText(
                "Rotation: " + std::to_string(m_currentRotation * 90) + " deg");
          }
        }
      }

      if (const auto *mb = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (isInMapViewport(mb->position)) {
          handleMapClick(*mb);
        }
      }

      if (const auto *mm = event->getIf<sf::Event::MouseMoved>()) {
        if (isInMapViewport(mm->position)) {
          m_hoveredGrid = pixelToGrid(mm->position);
          if (m_currentTool == EditorTool::Tiles &&
              sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            paintTileAt(m_hoveredGrid.x, m_hoveredGrid.y);
          }
        } else {
          m_hoveredGrid = {-1, -1};
        }
      }

      m_gui.handleEvent(*event);
    }

    m_window.clear(sf::Color(24, 25, 30));
    m_window.setView(m_mapView);

    // Draw map borders
    sf::RectangleShape borderRect(
        sf::Vector2f(static_cast<float>(m_map.width * m_map.tileSize.x),
                     static_cast<float>(m_map.height * m_map.tileSize.y)));
    borderRect.setFillColor(sf::Color::Transparent);
    borderRect.setOutlineColor(sf::Color(200, 50, 50, 200));
    borderRect.setOutlineThickness(2.f);
    m_window.draw(borderRect);

    m_window.draw(m_mapRenderer);
    drawSpawnOverlay();
    m_window.setView(m_window.getDefaultView());
    m_gui.draw();
    m_window.display();
  }
}

int MapEditorApp::mapPixelWidth() const {
  return static_cast<int>(m_map.width * m_map.tileSize.x);
}

int MapEditorApp::mapPixelHeight() const {
  return static_cast<int>(m_map.height * m_map.tileSize.y);
}

float MapEditorApp::mapViewportWidth() const {
  return std::max(1.f, static_cast<float>(m_window.getSize().x) - 340.f);
}

float MapEditorApp::mapViewportHeight() const {
  return static_cast<float>(m_window.getSize().y);
}

bool MapEditorApp::isInMapViewport(const sf::Vector2i &pixel) const {
  return pixel.x >= 0 && pixel.y >= 0 &&
         static_cast<float>(pixel.x) < mapViewportWidth() &&
         static_cast<float>(pixel.y) < mapViewportHeight();
}

bool MapEditorApp::inBounds(int gridX, int gridY) const {
  return gridX >= 0 && gridY >= 0 && gridX < static_cast<int>(m_map.width) &&
         gridY < static_cast<int>(m_map.height);
}

size_t MapEditorApp::tileIndex(int gridX, int gridY) const {
  return static_cast<size_t>(gridY * static_cast<int>(m_map.width) + gridX);
}

sf::Vector2i MapEditorApp::pixelToGrid(const sf::Vector2i &pixel) const {
  const sf::Vector2f world = m_window.mapPixelToCoords(pixel, m_mapView);
  return sf::Vector2i(
      static_cast<int>(world.x / static_cast<float>(m_map.tileSize.x)),
      static_cast<int>(world.y / static_cast<float>(m_map.tileSize.y)));
}

void MapEditorApp::updateMapView(float dt) {
  const float speed = 600.f;
  sf::Vector2f move(0.f, 0.f);
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
    move.x -= speed * dt;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
    move.x += speed * dt;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
    move.y -= speed * dt;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
    move.y += speed * dt;
  if (move.x != 0.f || move.y != 0.f) {
    m_mapView.move(move);
    clampMapView();
  }
}

void MapEditorApp::clampMapView() {
  m_mapView.setSize(
      {mapViewportWidth() * m_zoom, mapViewportHeight() * m_zoom});

  const float worldW = static_cast<float>(mapPixelWidth());
  const float worldH = static_cast<float>(mapPixelHeight());
  const sf::Vector2f half = m_mapView.getSize() * 0.5f;
  sf::Vector2f c = m_mapView.getCenter();
  const float padding =
      std::max(96.f, static_cast<float>(m_map.tileSize.x * 3));

  if (worldW <= m_mapView.getSize().x) {
    c.x = std::clamp(c.x, worldW * 0.5f - padding, worldW * 0.5f + padding);
  } else {
    c.x = std::clamp(c.x, half.x - padding, worldW - half.x + padding);
  }

  if (worldH <= m_mapView.getSize().y) {
    c.y = std::clamp(c.y, worldH * 0.5f - padding, worldH * 0.5f + padding);
  } else {
    c.y = std::clamp(c.y, half.y - padding, worldH - half.y + padding);
  }

  m_mapView.setCenter(c);
}

void MapEditorApp::refreshStatus(const std::string &message) {
  if (m_statusLabel)
    m_statusLabel->setText(message);
}

void MapEditorApp::rebuildRenderer() {
  m_tilesetTex.loadFromFile(m_map.tilesetPath);
  m_mapRenderer.load(m_map.tilesetPath, m_map.tileSize, m_map.tiles,
                     m_map.width, m_map.height);
}

void MapEditorApp::rebuildTileList() {
  if (!m_tilePicker)
    return;
  m_tilePicker->removeAllWidgets();
  m_activeTileBtn = nullptr;
  if (m_tilesetTex.getSize().x == 0 || m_map.tileSize.x == 0)
    return;

  const unsigned int sheetCols = m_tilesetTex.getSize().x / m_map.tileSize.x;
  const unsigned int total = (m_tilesetTex.getSize().x / m_map.tileSize.x) *
                             (m_tilesetTex.getSize().y / m_map.tileSize.y);
  const int tw = static_cast<int>(m_map.tileSize.x);
  const int th = static_cast<int>(m_map.tileSize.y);

  constexpr int btnSize = 48;
  constexpr int perRow = 5;
  constexpr float pad = 4.f;

  for (unsigned int i = 0; i < total; ++i) {
    const int id = static_cast<int>(i) + 1;
    const int srcX = static_cast<int>((i % sheetCols) * m_map.tileSize.x);
    const int srcY = static_cast<int>((i / sheetCols) * m_map.tileSize.y);

    tgui::UIntRect partRect(
        static_cast<unsigned>(srcX), static_cast<unsigned>(srcY),
        static_cast<unsigned>(tw), static_cast<unsigned>(th));
    tgui::Texture tileTex(m_map.tilesetPath.c_str(), partRect);

    auto btn = tgui::Button::create();
    btn->setSize(btnSize, btnSize);
    btn->setText("");
    btn->setPosition(pad + (i % perRow) * (btnSize + pad),
                     pad + (i / perRow) * (btnSize + pad));
    btn->getRenderer()->setTexture(tileTex);
    btn->getRenderer()->setTextureHover(tileTex);
    btn->getRenderer()->setTextureDown(tileTex);
    btn->getRenderer()->setBackgroundColor(sf::Color::Transparent);
    btn->getRenderer()->setBackgroundColorHover(sf::Color(255, 255, 255, 30));
    btn->getRenderer()->setBackgroundColorDown(sf::Color(255, 255, 255, 60));
    btn->getRenderer()->setBorders({2, 2, 2, 2});
    btn->getRenderer()->setBorderColor(id == m_selectedArtId
                                           ? sf::Color(80, 180, 255)
                                           : sf::Color(50, 50, 55));

    if (id == m_selectedArtId)
      m_activeTileBtn = btn;

    btn->onPress([this, id, btn]() {
      m_currentTool = EditorTool::Tiles;
      if (m_activeTileBtn)
        m_activeTileBtn->getRenderer()->setBorderColor(sf::Color(50, 50, 55));
      if (m_activeUnitBtn)
        m_activeUnitBtn->getRenderer()->setBorderColor(sf::Color(50, 50, 55));
      if (m_buildingList)
        m_buildingList->deselectItem();

      m_selectedArtId = id;
      m_selectedTerrain = terrainForTileId(id);
      if (m_terrainCombo) {
        m_terrainCombo->setSelectedItemById(terrainLabel(m_selectedTerrain));
      }
      m_activeTileBtn = btn;
      btn->getRenderer()->setBorderColor(sf::Color(80, 180, 255));
      refreshStatus("Tile " + std::to_string(id) + tileDescription(id) + " [" +
                    terrainLabel(m_selectedTerrain) + "]");
    });

    m_tilePicker->add(btn);
  }

  const unsigned int numRows = (total + perRow - 1) / perRow;
  m_tilePicker->setContentSize(
      {static_cast<float>(perRow * (btnSize + pad) + pad),
       static_cast<float>(numRows * (btnSize + pad) + pad)});
}

void MapEditorApp::rebuildUnitPicker() {
  if (!m_unitPicker)
    return;
  m_unitPicker->removeAllWidgets();
  m_activeUnitBtn = nullptr;

  constexpr int btnSize = 56;
  constexpr int perRow = 4;
  constexpr float pad = 4.f;

  for (size_t i = 0; i < m_unitTypes.size(); ++i) {
    const std::string &unitType = m_unitTypes[i];
    auto it = m_unitPreviews.find(unitType);
    if (it == m_unitPreviews.end())
      continue;

    const sf::IntRect &r = it->second.rect;
    tgui::UIntRect partRect(static_cast<unsigned>(r.position.x),
                            static_cast<unsigned>(r.position.y),
                            static_cast<unsigned>(r.size.x),
                            static_cast<unsigned>(r.size.y));
    tgui::Texture tex(it->second.texture, partRect);

    auto btn = tgui::Button::create();
    btn->setSize(btnSize, btnSize);
    btn->setText("");
    btn->setPosition(pad + static_cast<float>(i % perRow) * (btnSize + pad),
                     pad + static_cast<float>(i / perRow) * (btnSize + pad));
    btn->getRenderer()->setTexture(tex);
    btn->getRenderer()->setTextureHover(tex);
    btn->getRenderer()->setTextureDown(tex);
    btn->getRenderer()->setBackgroundColor(sf::Color::Transparent);
    btn->getRenderer()->setBackgroundColorHover(sf::Color(255, 255, 255, 30));
    btn->getRenderer()->setBackgroundColorDown(sf::Color(255, 255, 255, 60));
    btn->getRenderer()->setBorders({2, 2, 2, 2});
    btn->getRenderer()->setBorderColor(unitType == m_selectedUnitType
                                           ? sf::Color(80, 180, 255)
                                           : sf::Color(50, 50, 55));

    if (unitType == m_selectedUnitType)
      m_activeUnitBtn = btn;

    btn->onPress([this, btn, unitType]() {
      m_currentTool = EditorTool::Units;
      if (m_activeTileBtn)
        m_activeTileBtn->getRenderer()->setBorderColor(sf::Color(50, 50, 55));
      if (m_activeUnitBtn)
        m_activeUnitBtn->getRenderer()->setBorderColor(sf::Color(50, 50, 55));
      if (m_buildingList)
        m_buildingList->deselectItem();

      m_selectedUnitType = unitType;
      m_activeUnitBtn = btn;
      btn->getRenderer()->setBorderColor(sf::Color(80, 180, 255));
      refreshStatus("Selected unit " + unitType);
    });

    m_unitPicker->add(btn);
  }

  const unsigned int rows =
      static_cast<unsigned int>((m_unitTypes.size() + perRow - 1) / perRow);
  m_unitPicker->setContentSize(
      {static_cast<float>(perRow * (btnSize + pad) + pad),
       static_cast<float>(rows * (btnSize + pad) + pad)});
}

void MapEditorApp::loadSpritePreviews() {
  struct Info {
    std::string type;
    std::string path;
    sf::IntRect rect;
  };
  const std::vector<Info> unitInfos = {
      {"Tank", "Art/Units/Tank/tank_idle.png", {{0, 0}, {32, 32}}},
      {"Soldier", "Art/Units/Soldier/soldier_walk.png", {{0, 0}, {32, 32}}},
      {"MissileTank",
       "Art/Units/MissileTank/missiletank.png",
       {{0, 0}, {32, 32}}},
      {"Plane", "Art/Units/Plane/plane_idle.png", {{0, 0}, {32, 32}}},
      {"Destroyer",
       "Art/Units/Destroyer/destroyer_move.png",
       {{0, 0}, {32, 32}}}};
  for (const auto &info : unitInfos) {
    auto &preview = m_unitPreviews[info.type];
    preview.texture.loadFromFile(info.path);
    preview.rect = info.rect;
  }
  m_buildingPreview.texture.loadFromFile("Art/Buildings/buildings.png");
  m_buildingPreview.rect = {{0, 0}, {32, 32}};
}

int MapEditorApp::buildingCellIndex(const std::string &typeName) const {
  if (typeName == "HQ")
    return 0;
  if (typeName == "Factory")
    return 1;
  if (typeName == "Port")
    return 2;
  return 3;
}

bool MapEditorApp::loadMap(const std::string &path) {
  MapFile loaded;
  if (!FileManager::loadMap(path, loaded))
    return false;
  if (loaded.width == 0 || loaded.height == 0 || loaded.tiles.empty())
    return false;
  m_map = std::move(loaded);
  if (m_widthEdit)
    m_widthEdit->setText(std::to_string(m_map.width));
  if (m_heightEdit)
    m_heightEdit->setText(std::to_string(m_map.height));
  rebuildRenderer();
  rebuildTileList();
  clampMapView();
  return true;
}

bool MapEditorApp::saveMap(const std::string &path) {
  std::filesystem::path p(path);
  if (p.has_parent_path())
    std::filesystem::create_directories(p.parent_path());
  return FileManager::saveMap(m_map, path);
}

void MapEditorApp::buildUi() {
  auto mainPanel = tgui::ScrollablePanel::create();
  mainPanel->setPosition(mapViewportWidth(), 0.f);
  mainPanel->setSize(
      static_cast<float>(m_window.getSize().x - mapViewportWidth()),
      static_cast<float>(m_window.getSize().y));
  mainPanel->getRenderer()->setBackgroundColor(sf::Color(32, 34, 40));
  m_gui.add(mainPanel);

  auto panel = mainPanel; // for compatibility with rest of the code

  float y = 8.f;

  auto title = tgui::Label::create("Map Editor");
  title->setPosition(12, y);
  title->setTextSize(18);
  panel->add(title);
  y += 26.f;

  auto lbl = [&](const std::string &t) {
    auto w = tgui::Label::create(t);
    w->setPosition(12, y);
    panel->add(w);
    y += 20.f;
  };

  lbl("Map path");
  m_mapPathEdit = tgui::EditBox::create();
  m_mapPathEdit->setText(DEFAULT_MAP_PATH);
  m_mapPathEdit->setPosition(12, y);
  m_mapPathEdit->setSize("&.w - 24", 26);
  panel->add(m_mapPathEdit);
  y += 30.f;

  auto loadBtn = tgui::Button::create("Load");
  loadBtn->setPosition(12, y);
  loadBtn->setSize(80, 26);
  loadBtn->onPress([this]() {
    const std::string path = m_mapPathEdit->getText().toStdString();
    if (loadMap(path))
      refreshStatus("Loaded " + path);
    else
      refreshStatus("Load failed: " + path);
  });
  panel->add(loadBtn);

  auto saveBtn = tgui::Button::create("Save");
  saveBtn->setPosition(96, y);
  saveBtn->setSize(80, 26);
  saveBtn->onPress([this]() {
    const std::string path = m_mapPathEdit->getText().toStdString();
    if (saveMap(path))
      refreshStatus("Saved " + path);
    else
      refreshStatus("Save failed: " + path);
  });
  panel->add(saveBtn);

  auto newBtn = tgui::Button::create("New");
  newBtn->setPosition(180, y);
  newBtn->setSize(80, 26);
  newBtn->onPress([this]() {
    const int w =
        std::max(1, std::atoi(m_widthEdit->getText().toStdString().c_str()));
    const int h =
        std::max(1, std::atoi(m_heightEdit->getText().toStdString().c_str()));
    m_map = createDefaultMap();
    m_map.width = static_cast<unsigned>(w);
    m_map.height = static_cast<unsigned>(h);
    m_map.tiles.assign(m_map.width * m_map.height, Tile(2, TerrainType::Grass));
    rebuildRenderer();
    rebuildTileList();
    clampMapView();
    refreshStatus("New map " + std::to_string(w) + "x" + std::to_string(h));
  });
  panel->add(newBtn);
  y += 34.f;

  lbl("Map size (W x H)");
  m_widthEdit = tgui::EditBox::create();
  m_widthEdit->setText(std::to_string(m_map.width));
  m_widthEdit->setInputValidator(tgui::EditBox::Validator::UInt);
  m_widthEdit->setPosition(12, y);
  m_widthEdit->setSize("(&.w - 28) / 2", 26);
  panel->add(m_widthEdit);
  m_heightEdit = tgui::EditBox::create();
  m_heightEdit->setText(std::to_string(m_map.height));
  m_heightEdit->setInputValidator(tgui::EditBox::Validator::UInt);
  m_heightEdit->setPosition("12 + (&.w - 28) / 2 + 4", y);
  m_heightEdit->setSize("(&.w - 28) / 2", 26);
  panel->add(m_heightEdit);
  y += 30.f;

  auto updateMapBtn = tgui::Button::create("Update Map Size");
  updateMapBtn->setPosition(12, y);
  updateMapBtn->setSize("&.w - 24", 26);
  updateMapBtn->onPress([this]() {
    const unsigned int newW =
        std::max(1u, static_cast<unsigned int>(
                         std::stoul(m_widthEdit->getText().toStdString())));
    const unsigned int newH =
        std::max(1u, static_cast<unsigned int>(
                         std::stoul(m_heightEdit->getText().toStdString())));

    std::vector<Tile> newTiles(newW * newH, Tile(2, TerrainType::Grass));
    for (unsigned int gy = 0; gy < std::min(m_map.height, newH); ++gy) {
      for (unsigned int gx = 0; gx < std::min(m_map.width, newW); ++gx) {
        newTiles[gy * newW + gx] = m_map.tiles[gy * m_map.width + gx];
      }
    }
    m_map.tiles = std::move(newTiles);
    m_map.width = newW;
    m_map.height = newH;

    // Remove out of bounds spawns
    auto removeOob = [&](auto &list) {
      list.erase(std::remove_if(list.begin(), list.end(),
                                [&](const auto &s) {
                                  return s.gridX >= static_cast<int>(newW) ||
                                         s.gridY >= static_cast<int>(newH);
                                }),
                 list.end());
    };
    removeOob(m_map.spawns);
    removeOob(m_map.buildingSpawns);

    rebuildRenderer();
    clampMapView();
    refreshStatus("Updated map size to " + std::to_string(newW) + "x" +
                  std::to_string(newH));
  });
  panel->add(updateMapBtn);
  y += 34.f;

  lbl("Placement team");
  m_teamCombo = tgui::ComboBox::create();
  m_teamCombo->setPosition(12, y);
  m_teamCombo->setSize("&.w - 24", 26);
  m_teamCombo->onItemSelect([this](const tgui::String &) {
    const std::string idStr = m_teamCombo->getSelectedItemId().toStdString();
    if (!idStr.empty())
      m_selectedTeam = static_cast<Team>(std::stoi(idStr));
  });
  panel->add(m_teamCombo);
  y += 32.f;

  lbl("Terrain type");
  m_terrainCombo = tgui::ComboBox::create();
  m_terrainCombo->setPosition(12, y);
  m_terrainCombo->setSize("&.w - 24", 26);
  for (const char *t : {"Grass", "Road", "Mountain", "Water", "Forest"})
    m_terrainCombo->addItem(t, t);
  m_terrainCombo->setSelectedItemById("Grass");
  m_terrainCombo->onItemSelect([this](const tgui::String &) {
    m_selectedTerrain =
        parseTerrain(m_terrainCombo->getSelectedItemId().toStdString());
  });
  panel->add(m_terrainCombo);
  y += 32.f;

  m_autotileCheckbox = tgui::CheckBox::create();
  m_autotileCheckbox->setPosition(12, y);
  m_autotileCheckbox->setText("Enable Autotiling");
  m_autotileCheckbox->setChecked(true);
  panel->add(m_autotileCheckbox);
  y += 26.f;

  m_rotationLabel = tgui::Label::create("Rotation: 0 deg");
  m_rotationLabel->setPosition(12, y);
  m_rotationLabel->setTextSize(13);
  m_rotationLabel->getRenderer()->setTextColor(sf::Color(180, 180, 180));
  panel->add(m_rotationLabel);
  y += 22.f;

  lbl("Tile");
  m_tilePicker = tgui::ScrollablePanel::create();
  m_tilePicker->setPosition(12, y);
  m_tilePicker->setSize("&.w - 24", 220);
  m_tilePicker->getRenderer()->setBackgroundColor(sf::Color(20, 22, 26));
  panel->add(m_tilePicker);
  rebuildTileList();
  y += 226.f;

  lbl("Units");
  m_unitPicker = tgui::ScrollablePanel::create();
  m_unitPicker->setPosition(12, y);
  m_unitPicker->setSize("&.w - 24", 124);
  m_unitPicker->getRenderer()->setBackgroundColor(sf::Color(20, 22, 26));
  panel->add(m_unitPicker);
  rebuildUnitPicker();
  y += 130.f;

  lbl("Buildings");
  m_buildingList = tgui::ListBox::create();
  m_buildingList->setPosition(12, y);
  m_buildingList->setSize("&.w - 24", 90);
  for (const auto &t : m_buildingTypes)
    m_buildingList->addItem(t, t);
  m_buildingList->setSelectedItemById(m_selectedBuildingType);
  m_buildingList->onItemSelect([this](const tgui::String &) {
    m_currentTool = EditorTool::Buildings;
    if (m_activeTileBtn)
      m_activeTileBtn->getRenderer()->setBorderColor(sf::Color(50, 50, 55));
    if (m_activeUnitBtn)
      m_activeUnitBtn->getRenderer()->setBorderColor(sf::Color(50, 50, 55));

    const std::string id = m_buildingList->getSelectedItemId().toStdString();
    if (!id.empty())
      m_selectedBuildingType = id;
  });
  panel->add(m_buildingList);
  y += 96.f;

  auto hint = tgui::Label::create(
      "LMB place  RMB remove  Drag paint  WASD/Arrows move camera");
  hint->setPosition(12, y);
  hint->setTextSize(13);
  panel->add(hint);
  y += 22.f;

  m_statusLabel = tgui::Label::create("Ready");
  m_statusLabel->setPosition(12, y);
  m_statusLabel->setTextSize(13);
  panel->add(m_statusLabel);
  y += 30.f;

  lbl("Teams Settings");

  auto addTeamBtn = tgui::Button::create("Add Team");
  addTeamBtn->setPosition(12, y);
  addTeamBtn->setSize("&.w / 2 - 16", 26);
  addTeamBtn->onPress([this]() {
    TeamData t;
    t.team = static_cast<Team>(m_map.teams.size());
    t.name = "New Team";
    t.startMoney = 1000;
    t.color = sf::Color::White;
    m_map.teams.push_back(t);
    rebuildTeamEditor();
  });
  panel->add(addTeamBtn);

  auto remTeamBtn = tgui::Button::create("Remove Team");
  remTeamBtn->setPosition("12 + &.w / 2 - 12", y);
  remTeamBtn->setSize("&.w / 2 - 16", 26);
  remTeamBtn->onPress([this]() {
    if (!m_map.teams.empty()) {
      m_map.teams.pop_back();
      rebuildTeamEditor();
    }
  });
  panel->add(remTeamBtn);
  y += 34.f;

  m_teamEditorPanel = tgui::ScrollablePanel::create();
  m_teamEditorPanel->setPosition(12, y);
  m_teamEditorPanel->setSize("&.w - 24", 250);
  m_teamEditorPanel->getRenderer()->setBackgroundColor(sf::Color(20, 22, 26));
  panel->add(m_teamEditorPanel);
  rebuildTeamEditor();
}

void MapEditorApp::rebuildTeamEditor() {
  if (m_teamCombo) {
    m_teamCombo->removeAllItems();
    for (const auto &td : m_map.teams) {
      m_teamCombo->addItem(td.name, std::to_string(static_cast<int>(td.team)));
    }
    if (!m_map.teams.empty()) {
      m_teamCombo->setSelectedItemById(
          std::to_string(static_cast<int>(m_map.teams.front().team)));
    }
  }

  if (!m_teamEditorPanel)
    return;
  m_teamEditorPanel->removeAllWidgets();

  float ty = 4.f;
  for (size_t i = 0; i < m_map.teams.size(); ++i) {
    auto &td = m_map.teams[i];

    auto lbl = tgui::Label::create("Team " + std::to_string(i + 1));
    lbl->setPosition(4, ty);
    lbl->setTextSize(14);
    m_teamEditorPanel->add(lbl);
    ty += 20.f;

    auto nameEdit = tgui::EditBox::create();
    nameEdit->setPosition(4, ty);
    nameEdit->setSize("&.w - 20", 22);
    nameEdit->setText(td.name);
    nameEdit->onTextChange([this, &td](const tgui::String &t) {
      td.name = t.toStdString();
      if (m_teamCombo) {
        m_teamCombo->changeItemById(std::to_string(static_cast<int>(td.team)),
                                    t);
      }
    });
    m_teamEditorPanel->add(nameEdit);
    ty += 26.f;

    auto cashLbl = tgui::Label::create("Cash:");
    cashLbl->setPosition(4, ty);
    m_teamEditorPanel->add(cashLbl);
    auto cashEdit = tgui::EditBox::create();
    cashEdit->setPosition(50, ty);
    cashEdit->setSize(60, 22);
    cashEdit->setText(std::to_string(td.startMoney));
    cashEdit->setInputValidator(tgui::EditBox::Validator::UInt);
    cashEdit->onTextChange([&td](const tgui::String &t) {
      if (!t.empty())
        td.startMoney = std::stoi(t.toStdString());
    });
    m_teamEditorPanel->add(cashEdit);

    auto rEdit = tgui::EditBox::create();
    rEdit->setPosition(120, ty);
    rEdit->setSize(30, 22);
    rEdit->setText(std::to_string(td.color.r));
    auto gEdit = tgui::EditBox::create();
    gEdit->setPosition(155, ty);
    gEdit->setSize(30, 22);
    gEdit->setText(std::to_string(td.color.g));
    auto bEdit = tgui::EditBox::create();
    bEdit->setPosition(190, ty);
    bEdit->setSize(30, 22);
    bEdit->setText(std::to_string(td.color.b));

    auto colorBox = tgui::Panel::create();
    colorBox->setPosition(225, ty);
    colorBox->setSize(22, 22);
    colorBox->getRenderer()->setBackgroundColor(td.color);
    colorBox->getRenderer()->setBorders(1);
    colorBox->getRenderer()->setBorderColor(sf::Color::White);
    m_teamEditorPanel->add(colorBox);

    auto updateColor = [&td, rEdit, gEdit, bEdit, colorBox]() {
      if (!rEdit->getText().empty())
        td.color.r =
            std::clamp(std::stoi(rEdit->getText().toStdString()), 0, 255);
      if (!gEdit->getText().empty())
        td.color.g =
            std::clamp(std::stoi(gEdit->getText().toStdString()), 0, 255);
      if (!bEdit->getText().empty())
        td.color.b =
            std::clamp(std::stoi(bEdit->getText().toStdString()), 0, 255);
      colorBox->getRenderer()->setBackgroundColor(td.color);
    };
    rEdit->onTextChange(updateColor);
    gEdit->onTextChange(updateColor);
    bEdit->onTextChange(updateColor);
    m_teamEditorPanel->add(rEdit);
    m_teamEditorPanel->add(gEdit);
    m_teamEditorPanel->add(bEdit);

    ty += 34.f;
  }
}

void MapEditorApp::handleMapClick(const sf::Event::MouseButtonPressed &mb) {
  const sf::Vector2i g = pixelToGrid(mb.position);
  const int gx = g.x;
  const int gy = g.y;
  if (!inBounds(gx, gy))
    return;
  if (mb.button == sf::Mouse::Button::Left)
    placeAt(gx, gy);
  else if (mb.button == sf::Mouse::Button::Right)
    removeAt(gx, gy);
}

void MapEditorApp::runAutotile(int gx, int gy) {
  auto processTile = [&](int x, int y) {
    if (!inBounds(x, y))
      return;
    Tile &t = m_map.tiles[tileIndex(x, y)];
    if (t.getTerrain() != TerrainType::Grass)
      return;

    bool wTop =
        inBounds(x, y - 1) &&
        m_map.tiles[tileIndex(x, y - 1)].getTerrain() == TerrainType::Water;
    bool wBottom =
        inBounds(x, y + 1) &&
        m_map.tiles[tileIndex(x, y + 1)].getTerrain() == TerrainType::Water;
    bool wLeft =
        inBounds(x - 1, y) &&
        m_map.tiles[tileIndex(x - 1, y)].getTerrain() == TerrainType::Water;
    bool wRight =
        inBounds(x + 1, y) &&
        m_map.tiles[tileIndex(x + 1, y)].getTerrain() == TerrainType::Water;

    int newId = t.getArtId();
    if (newId >= 12 && newId <= 19)
      newId = 2; // reset edges to default grass

    int mask =
        (wTop ? 1 : 0) | (wBottom ? 2 : 0) | (wLeft ? 4 : 0) | (wRight ? 8 : 0);
    switch (mask) {
    case 1:
      newId = 15;
      break;
    case 2:
      newId = 14;
      break;
    case 4:
      newId = 13;
      break;
    case 8:
      newId = 12;
      break;
    case 5:
      newId = 15;
      break;
    case 9:
      newId = 15;
      break;
    case 6:
      newId = 14;
      break;
    case 10:
      newId = 14;
      break;
    case 0: {
      bool wTopLeft = inBounds(x - 1, y - 1) &&
                      m_map.tiles[tileIndex(x - 1, y - 1)].getTerrain() ==
                          TerrainType::Water;
      bool wTopRight = inBounds(x + 1, y - 1) &&
                       m_map.tiles[tileIndex(x + 1, y - 1)].getTerrain() ==
                           TerrainType::Water;
      bool wBottomLeft = inBounds(x - 1, y + 1) &&
                         m_map.tiles[tileIndex(x - 1, y + 1)].getTerrain() ==
                             TerrainType::Water;
      bool wBottomRight = inBounds(x + 1, y + 1) &&
                          m_map.tiles[tileIndex(x + 1, y + 1)].getTerrain() ==
                              TerrainType::Water;

      if (wBottomRight)
        newId = 16;
      else if (wTopRight)
        newId = 17;
      else if (wBottomLeft)
        newId = 18;
      else if (wTopLeft)
        newId = 19;
      break;
    }
    default:
      break;
    }

    if (newId != t.getArtId()) {
      t = Tile(newId, TerrainType::Grass);
    }
  };

  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      processTile(gx + dx, gy + dy);
    }
  }
}

void MapEditorApp::paintTileAt(int gx, int gy) {
  if (!inBounds(gx, gy))
    return;
  m_map.tiles[tileIndex(gx, gy)] =
      Tile(m_selectedArtId, m_selectedTerrain, m_currentRotation);
  if (m_autotileCheckbox && m_autotileCheckbox->isChecked()) {
    runAutotile(gx, gy);
  }
  rebuildRenderer();
}

void MapEditorApp::placeAt(int gridX, int gridY) {
  if (m_currentTool == EditorTool::Tiles) {
    paintTileAt(gridX, gridY);
    return;
  }

  if (m_currentTool == EditorTool::Units) {
    auto it =
        std::find_if(m_map.spawns.begin(), m_map.spawns.end(),
                     [&](const UnitSpawnData &spawn) {
                       return spawn.gridX == gridX && spawn.gridY == gridY;
                     });
    if (it == m_map.spawns.end()) {
      UnitSpawnData spawn;
      spawn.typeName = m_selectedUnitType;
      spawn.gridX = gridX;
      spawn.gridY = gridY;
      spawn.team = m_selectedTeam;
      m_map.spawns.push_back(spawn);
    } else {
      it->typeName = m_selectedUnitType;
      it->team = m_selectedTeam;
    }
    refreshStatus("Placed unit " + m_selectedUnitType + " for " +
                  teamLabel(m_selectedTeam));
    return;
  }

  auto it =
      std::find_if(m_map.buildingSpawns.begin(), m_map.buildingSpawns.end(),
                   [&](const BuildingSpawnData &spawn) {
                     return spawn.gridX == gridX && spawn.gridY == gridY;
                   });
  if (it == m_map.buildingSpawns.end()) {
    BuildingSpawnData spawn;
    spawn.typeName = m_selectedBuildingType;
    spawn.gridX = gridX;
    spawn.gridY = gridY;
    spawn.team = m_selectedTeam;
    m_map.buildingSpawns.push_back(spawn);
  } else {
    it->typeName = m_selectedBuildingType;
    it->team = m_selectedTeam;
  }
  refreshStatus("Placed building " + m_selectedBuildingType + " for " +
                teamLabel(m_selectedTeam));
}

void MapEditorApp::removeAt(int gridX, int gridY) {
  if (m_currentTool == EditorTool::Units) {
    auto endIt =
        std::remove_if(m_map.spawns.begin(), m_map.spawns.end(),
                       [&](const UnitSpawnData &spawn) {
                         return spawn.gridX == gridX && spawn.gridY == gridY;
                       });
    if (endIt != m_map.spawns.end()) {
      m_map.spawns.erase(endIt, m_map.spawns.end());
      refreshStatus("Removed unit spawn");
    }
  }

  if (m_currentTool == EditorTool::Buildings) {
    auto endIt =
        std::remove_if(m_map.buildingSpawns.begin(), m_map.buildingSpawns.end(),
                       [&](const BuildingSpawnData &spawn) {
                         return spawn.gridX == gridX && spawn.gridY == gridY;
                       });
    if (endIt != m_map.buildingSpawns.end()) {
      m_map.buildingSpawns.erase(endIt, m_map.buildingSpawns.end());
      refreshStatus("Removed building spawn");
    }
  }
}

void MapEditorApp::drawSpawnOverlay() {
  const float tileW = static_cast<float>(m_map.tileSize.x);
  const float tileH = static_cast<float>(m_map.tileSize.y);
  const int cellSz = static_cast<int>(m_map.tileSize.x);

  for (const auto &spawn : m_map.buildingSpawns) {
    sf::Sprite sprite(m_buildingPreview.texture);
    sprite.setTextureRect(sf::IntRect(
        {buildingCellIndex(spawn.typeName) * cellSz, 0}, {cellSz, cellSz}));
    sf::Color tint = TeamRegistry::getColor(spawn.team);
    tint.a = 220;
    sprite.setColor(tint);
    sprite.setPosition({spawn.gridX * tileW, spawn.gridY * tileH});
    m_window.draw(sprite);
  }

  for (const auto &spawn : m_map.spawns) {
    auto it = m_unitPreviews.find(spawn.typeName);
    if (it == m_unitPreviews.end())
      continue;
    sf::Sprite sprite(it->second.texture);
    sprite.setTextureRect(it->second.rect);
    sf::Color tint = TeamRegistry::getColor(spawn.team);
    tint.a = 220;
    sprite.setColor(tint);

    sprite.setOrigin({tileW / 2.f, tileH / 2.f});
    sprite.setPosition(
        {spawn.gridX * tileW + tileW / 2.f, spawn.gridY * tileH + tileH / 2.f});
    if (spawn.facingDirection == MoveDirection::Left)
      sprite.setScale({-1.f, 1.f});

    m_window.draw(sprite);
  }

  if (m_hoveredGrid.x >= 0 && m_hoveredGrid.x < static_cast<int>(m_map.width) &&
      m_hoveredGrid.y >= 0 &&
      m_hoveredGrid.y < static_cast<int>(m_map.height)) {

    sf::Vector2f centerPos(m_hoveredGrid.x * tileW + tileW / 2.f,
                           m_hoveredGrid.y * tileH + tileH / 2.f);
    sf::Color ghostTint = sf::Color(255, 255, 255, 140);

    if (m_currentTool == EditorTool::Tiles) {
      int id = m_selectedArtId;
      if (id > 0) {
        int tu = (id - 1) % (m_tilesetTex.getSize().x / m_map.tileSize.x);
        int tv = (id - 1) / (m_tilesetTex.getSize().x / m_map.tileSize.x);
        sf::Sprite ghost(m_tilesetTex);
        ghost.setTextureRect(
            sf::IntRect({tu * cellSz, tv * cellSz}, {cellSz, cellSz}));
        ghost.setOrigin({tileW / 2.f, tileH / 2.f});
        ghost.setPosition(centerPos);
        ghost.setRotation(sf::degrees(m_currentRotation * 90.f));
        ghost.setColor(ghostTint);
        m_window.draw(ghost);
      }
    } else if (m_currentTool == EditorTool::Units) {
      auto it = m_unitPreviews.find(m_selectedUnitType);
      if (it != m_unitPreviews.end()) {
        sf::Sprite ghost(it->second.texture);
        ghost.setTextureRect(it->second.rect);
        ghostTint = TeamRegistry::getColor(m_selectedTeam);
        ghostTint.a = 140;
        ghost.setColor(ghostTint);
        ghost.setOrigin({tileW / 2.f, tileH / 2.f});
        ghost.setPosition(centerPos);
        MoveDirection dir = static_cast<MoveDirection>(m_currentRotation);
        if (dir == MoveDirection::Left)
          ghost.setScale({-1.f, 1.f});
        m_window.draw(ghost);
      }
    } else if (m_currentTool == EditorTool::Buildings) {
      sf::Sprite ghost(m_buildingPreview.texture);
      ghost.setTextureRect(
          sf::IntRect({buildingCellIndex(m_selectedBuildingType) * cellSz, 0},
                      {cellSz, cellSz}));
      ghostTint = TeamRegistry::getColor(m_selectedTeam);
      ghostTint.a = 140;
      ghost.setColor(ghostTint);
      ghost.setOrigin({tileW / 2.f, tileH / 2.f});
      ghost.setPosition(centerPos);
      m_window.draw(ghost);
    }
  }
}
