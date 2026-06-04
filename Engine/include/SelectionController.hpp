#pragma once

#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class MapManager;
class Unit;
class TurnController;

class SelectionController {
public:
    SelectionController(sf::RenderWindow& window, MapManager& mapManager, TurnController& turnController, const std::string& walkOverlayPath, const std::string& moveArrowPath, const std::string& iconsPath, const std::string& enemyOverlayPath);
    void handleEvent(const sf::Event& event);
    void drawOverlays(sf::RenderTarget& target);
    void drawCursorIcon(sf::RenderTarget& target);
    void drawGui();
    void clearSelection();
    void selectUnit(std::shared_ptr<Unit> unit);
    void syncCameraView(sf::View gameView);
    tgui::Gui& getGui() { return gui; }
    std::shared_ptr<Unit> getSelectedUnit() const { return selectedUnit; }

private:
    void updateAttackPath(sf::Vector2i enemyGrid, sf::Vector2i unitGrid, sf::Vector2i preferredDir);

    tgui::Gui gui;
    sf::RenderWindow& m_window;
    MapManager& mapManager;
    TurnController& m_turnController;
    std::shared_ptr<Unit> selectedUnit;
    std::shared_ptr<Unit> hoveredEnemyUnit;
    std::vector<sf::Vector2i> reachableTiles;
    std::vector<sf::Vector2i> previewPath;
    sf::Texture m_walkOverlayTexture;
    sf::Texture m_moveArrowTexture;
    sf::Texture m_iconsTexture;
    sf::Texture m_enemyOverlayTexture;
    sf::Vector2f m_cursorPos;
    int m_cursorIconCell{-1};
    sf::Vector2i m_preferredApproachDir{0, 0};
    float m_scaleX{1.f};
    float m_scaleY{1.f};
    std::vector<tgui::Button::Ptr> m_tileButtons;
    tgui::Label::Ptr m_turnLabel;
};

