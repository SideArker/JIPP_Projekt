#pragma once

#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class MapManager;
class Unit;

class SelectionController {
public:
    SelectionController(sf::RenderWindow& window, MapManager& mapManager);
    void handleEvent(const sf::Event& event);
    void drawOverlays(sf::RenderTarget& target) const;
    void drawGui();

private:
    tgui::Gui gui;
    MapManager& mapManager;
    std::shared_ptr<Unit> selectedUnit;
    std::vector<sf::Vector2i> reachableTiles;
    std::vector<sf::Vector2i> previewPath;
    sf::Texture m_walkOverlayTexture;
};
