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
    SelectionController(sf::RenderWindow& window, MapManager& mapManager, const std::string& walkOverlayPath);
    void handleEvent(const sf::Event& event);
    void drawOverlays(sf::RenderTarget& target) const;
    void drawGui();

private:
    void updateAttackPath(sf::Vector2i enemyGrid, sf::Vector2i unitGrid, sf::Vector2i preferredDir);

    tgui::Gui gui;
    MapManager& mapManager;
    std::shared_ptr<Unit> selectedUnit;
    std::shared_ptr<Unit> hoveredEnemyUnit;
    std::vector<sf::Vector2i> reachableTiles;
    std::vector<sf::Vector2i> previewPath;
    sf::Texture m_walkOverlayTexture;
    sf::Vector2i m_preferredApproachDir{0, 0};
    float m_scaleX{1.f};
    float m_scaleY{1.f};
};
