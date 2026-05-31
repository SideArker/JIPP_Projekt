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
    SelectionController(sf::RenderWindow& window, MapManager& mapManager, const std::string& walkOverlayPath, const std::string& moveArrowPath, const std::string& iconsPath);
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
    sf::Texture m_moveArrowTexture;
    sf::Texture m_iconsTexture;
    sf::Vector2f m_cursorPos;
    int m_cursorIconCell{-1}; // -1 = hidden, 0 = walk, 1 = shoot
    sf::Vector2i m_preferredApproachDir{0, 0};
    float m_scaleX{1.f};
    float m_scaleY{1.f};
};
