#pragma once
#include "MapEditorTypes.hpp"
#include "MapRenderer.hpp"
#include "TeamRegistry.hpp"
#include <SFML/Graphics.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <map>
#include <string>
#include <vector>

class MapEditorApp {
public:
  MapEditorApp();
  void run();

private:
  struct SpritePreview {
    sf::Texture texture;
    sf::IntRect rect;
  };

  sf::RenderWindow m_window;
  tgui::Gui m_gui;
  sf::View m_mapView;
  MapFile m_map;
  MapRenderer m_mapRenderer;
  sf::Texture m_tilesetTex;

  std::vector<std::string> m_unitTypes = {
      "Tank",      "Soldier", "MissileTank", "Plane",
      "Destroyer", "Turret",  "Barricade"};
  std::vector<std::string> m_buildingTypes = {
      "HQ",         "Airport", "Factory",  "VehicleBase",
      "LandOilRig", "Port",    "SeaOilRig"};

  EditorTool m_currentTool = EditorTool::Tiles;
  int m_selectedArtId = 1;
  TerrainType m_selectedTerrain = TerrainType::Grass;
  std::string m_selectedUnitType = "Tank";
  std::string m_selectedBuildingType = "HQ";
  Team m_selectedTeam = Team::Ally;

  unsigned char m_currentRotation{0};
  tgui::Label::Ptr m_rotationLabel;

  bool m_dragging{false};
  sf::Vector2i m_dragStartPos;
  sf::Vector2f m_cameraViewCenter;
  float m_zoom{1.0f};

  sf::Vector2i m_hoveredGrid{-1, -1};

  tgui::ComboBox::Ptr m_toolCombo;
  tgui::ComboBox::Ptr m_teamCombo;
  tgui::ComboBox::Ptr m_terrainCombo;
  tgui::ScrollablePanel::Ptr m_tilePicker;
  tgui::Button::Ptr m_activeTileBtn;
  tgui::ScrollablePanel::Ptr m_unitPicker;
  tgui::Button::Ptr m_activeUnitBtn;
  tgui::ListBox::Ptr m_buildingList;
  tgui::EditBox::Ptr m_mapPathEdit;
  tgui::EditBox::Ptr m_widthEdit;
  tgui::EditBox::Ptr m_heightEdit;
  tgui::Label::Ptr m_statusLabel;
  tgui::CheckBox::Ptr m_autotileCheckbox;

  std::map<std::string, SpritePreview> m_unitPreviews;
  SpritePreview m_buildingPreview;

  int mapPixelWidth() const;
  int mapPixelHeight() const;
  float mapViewportWidth() const;
  float mapViewportHeight() const;
  bool isInMapViewport(const sf::Vector2i &pixel) const;
  bool inBounds(int gridX, int gridY) const;
  size_t tileIndex(int gridX, int gridY) const;
  sf::Vector2i pixelToGrid(const sf::Vector2i &pixel) const;
  void updateMapView(float dt);
  void clampMapView();

  void refreshStatus(const std::string &message);
  void rebuildRenderer();
  void rebuildTileList();
  void rebuildUnitPicker();
  void loadSpritePreviews();
  int buildingCellIndex(const std::string &typeName) const;
  bool loadMap(const std::string &path);
  bool saveMap(const std::string &path);

  void buildUi();
  void rebuildTeamEditor();
  void handleMapClick(const sf::Event::MouseButtonPressed &mb);
  void paintTileAt(int gridX, int gridY);
  void runAutotile(int gx, int gy);
  void placeAt(int gridX, int gridY);
  void removeAt(int gridX, int gridY);
  void drawSpawnOverlay();

  tgui::ScrollablePanel::Ptr m_teamEditorPanel;
};
