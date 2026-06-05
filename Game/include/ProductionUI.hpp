#pragma once

#include "Building.hpp"
#include "MapManager.hpp"
#include "TurnController.hpp"
#include <SFML/Graphics.hpp>
#include <TGUI/Backend/Renderer/SFML-Graphics/CanvasSFML.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <memory>
#include <string>
#include <vector>

class ProductionUI {
public:
  struct AnimatedPreview {
    std::shared_ptr<Unit> unit;
    tgui::CanvasSFML::Ptr canvas;
    float timer = 0.f;
    int dirIndex = 0;
  };

  ProductionUI(tgui::Gui &gui, MapManager &mapManager,
               TurnController &turnController);

  void open(std::shared_ptr<Building> factory);
  void close();

  void update(float dt);

  bool isOpen() const { return m_panel != nullptr; }

private:
  tgui::Gui &m_gui;
  MapManager &m_mapManager;
  TurnController &m_turnController;

  tgui::Panel::Ptr m_panel;
  std::vector<AnimatedPreview> m_previews;

  static tgui::Button::Ptr makeStyledBtn(const std::string &text);
};
