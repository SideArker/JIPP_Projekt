#pragma once

#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "GameContent.hpp"
#include "RightPanelUI.hpp"
#include "MapManager.hpp"
#include "ProductionUI.hpp"
#include "CameraController.hpp"
#include <memory>
#include <map>

class GameUIManager {
public:
    GameUIManager(sf::RenderWindow& window, MapManager& mapManager, CameraController& cameraController);
    
    void handleEvent(const sf::Event& event);
    void update(float dt, MapManager& mapManager);
    void draw();
    
    tgui::Gui& getGui() { return m_gui; }
    ProductionUI* getProductionUI() { return m_productionUI.get(); }
    bool shouldQuitToMenu() const { return m_quitToMenu; }

private:
    bool m_quitToMenu = false;
    tgui::Gui m_gui;
    GameUIWidgets m_panels;
    std::unique_ptr<ProductionUI> m_productionUI;
    std::map<Team, tgui::Label::Ptr> m_teamMoneyLabels;
};
