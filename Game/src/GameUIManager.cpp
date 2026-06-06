#include "GameUIManager.hpp"
#include "SoundManager.hpp"
#include "SettingsManager.hpp"
#include <cstdlib>

GameUIManager::GameUIManager(sf::RenderWindow& window, MapManager& mapManager, CameraController& cameraController)
    : m_gui(window) {
    m_gui.setFont("Art/Fonts/joystixMonospace.ttf");
    m_panels = buildGameUI(m_gui);

    m_productionUI = std::make_unique<ProductionUI>(m_gui, mapManager, mapManager.getTurnController());
    
    mapManager.setOnOpenFactory([this](std::shared_ptr<Building> factory) {
        if (m_productionUI) {
            m_productionUI->open(factory);
        }
    });

    m_panels.endTurnBtn->onClick([&mapManager]() { mapManager.requestEndTurn(); });

    m_panels.undoBtn->onClick([&mapManager, &cameraController]() {
        std::shared_ptr<Unit> outUnit;
        if (mapManager.popUndoState(outUnit)) {
            if (outUnit) {
                cameraController.trackUnit(outUnit);
                mapManager.selectUnit(outUnit);
            }
        }
    });

    m_panels.nextUnitBtn->onClick([&mapManager, &cameraController]() {
        auto &units = mapManager.getUnits();
        std::vector<std::shared_ptr<Unit>> available;
        for (auto &u : units) {
            if (!u->isDead() && u->getTeam() == Team::Ally && !u->hasActed())
                available.push_back(u);
        }
        if (!available.empty()) {
            int idx = rand() % available.size();
            cameraController.trackUnit(available[idx]);
            mapManager.selectUnit(available[idx]);
        }
    });

    m_panels.settingsBtn->onClick([this]() {
        m_panels.settingsPanel->setVisible(!m_panels.settingsPanel->isVisible());
    });
    
    m_panels.closeSettingsBtn->onClick([this]() { m_panels.settingsPanel->setVisible(false); });
    
    m_panels.saveGameBtn->onClick([&mapManager]() { mapManager.saveToFile("savegame.sav"); });
    
    Settings settings = SettingsManager::load();
    m_panels.musicVolSlider->setValue(settings.musicVolume);
    m_panels.soundVolSlider->setValue(settings.soundVolume);
    m_panels.fullscreenCheckbox->setChecked(settings.fullscreen);

    m_panels.musicVolSlider->onValueChange([](float v) { 
        SoundManager::setMusicVolume(v); 
        Settings s = SettingsManager::load();
        s.musicVolume = static_cast<int>(v);
        SettingsManager::save(s);
    });
    m_panels.soundVolSlider->onValueChange([](float v) { 
        SoundManager::setSFXVolume(v); 
        Settings s = SettingsManager::load();
        s.soundVolume = static_cast<int>(v);
        SettingsManager::save(s);
    });
    m_panels.fullscreenCheckbox->onChange([&window](bool checked) {
        Settings s = SettingsManager::load();
        s.fullscreen = checked;
        SettingsManager::save(s);

        auto style = checked ? sf::State::Fullscreen : sf::State::Windowed;
        window.create(sf::VideoMode({1280, 720}), "Map Renderer", style);
    });

    m_panels.teamList->removeAllWidgets();
    int teamY = 0;
    for (const auto &[team, data] : mapManager.getTeams()) {
        if (team == Team::Neutral) continue;

        auto teamPanel = tgui::Panel::create({"100%", 45});
        teamPanel->setPosition(0, teamY);
        teamPanel->getRenderer()->setBackgroundColor(sf::Color(30, 30, 35, 180));
        m_panels.teamList->add(teamPanel);

        auto colorStrip = tgui::Panel::create({5, "100%"});
        colorStrip->setPosition(0, 0);
        colorStrip->getRenderer()->setBackgroundColor(data.color);
        teamPanel->add(colorStrip);

        auto nameLbl = tgui::Label::create();
        nameLbl->setText(data.name);
        nameLbl->getRenderer()->setTextColor(sf::Color::White);
        nameLbl->setTextSize(14);
        nameLbl->setMaximumTextWidth(140.f);
        nameLbl->setPosition(12, "50% - 15");
        teamPanel->add(nameLbl);

        auto moneyLbl = tgui::Label::create();
        moneyLbl->setText("$" + std::to_string(data.money));
        moneyLbl->getRenderer()->setTextColor(sf::Color(220, 220, 220));
        moneyLbl->setTextSize(14);
        moneyLbl->setPosition("100% - width - 10", "50% - 7");
        teamPanel->add(moneyLbl);
        m_teamMoneyLabels[team] = moneyLbl;

        teamY += 50;
    }

    mapManager.onUnitMoveStart = [&cameraController](std::shared_ptr<Unit> u) {
        cameraController.trackUnit(u);
    };

    mapManager.setOnSelectionChanged(
        [this, infoLabel = m_panels.infoLabel, flagsList = m_panels.flagsList, portrait = m_panels.unitPortrait](
            std::shared_ptr<Unit> unit, std::shared_ptr<Building> building,
            const Tile *tile) {
            flagsList->removeAllWidgets();
            portrait->setVisible(false);
            if (unit) {
                std::string desc = unit->getName() + "\n";
                desc += "HP: " + std::to_string(unit->getHealth()) + "/" + std::to_string(unit->getMaxHealth()) + "\n";
                desc += "Damage: " + std::to_string(unit->getDamage()) + "\n";
                desc += "Speed: " + std::to_string(unit->getMoveSpeed()) + "\n";
                desc += "Range: " + std::to_string(unit->getMinAttackRange()) + "-" + std::to_string(unit->getMaxAttackRange()) + "\n";
                infoLabel->setText(desc);

                try {
                    tgui::Texture tex(unit->getArtPath(), tgui::UIntRect(0, 0, 32, 32));
                    portrait->getRenderer()->setTexture(tex);
                    portrait->setVisible(true);
                } catch (...) {}

                int flagY = 0;
                if (unit->hasFlag(UnitFlag::Capture)) {
                    auto flagPanel = tgui::Panel::create({"100%", "36px"});
                    flagPanel->setPosition(0, flagY);
                    tgui::Texture tex("Art/UI/flag.png", tgui::UIntRect(0, 0, 32, 32));
                    auto pic = tgui::Picture::create(tex);
                    pic->setSize("32px", "32px");
                    pic->setPosition("2px", "2px");
                    flagPanel->add(pic);
                    auto lbl = tgui::Label::create("Can capture buildings");
                    lbl->setPosition("40px", "8px");
                    lbl->getRenderer()->setTextColor(sf::Color(200, 220, 255));
                    flagPanel->add(lbl);
                    
                    flagsList->add(flagPanel);
                    flagY += 40;
                }
            } else if (building) {
                std::string desc = building->getTypeName() + "\n";
                desc += building->getDescription() + "\n";
                desc += "Team: " + (building->getTeam() == Team::Ally ? std::string("Ally") : (building->getTeam() == Team::Enemy ? std::string("Enemy") : std::string("Neutral"))) + "\n";
                infoLabel->setText(desc);
            } else {
                infoLabel->setText("No selection");
            }
        });
}

void GameUIManager::handleEvent(const sf::Event& event) {
    m_gui.handleEvent(event);
}

void GameUIManager::update(float dt, MapManager& mapManager) {
    if (m_productionUI) {
        m_productionUI->update(dt);
    }

    for (auto &[t, lbl] : m_teamMoneyLabels) {
        auto it = mapManager.getTeams().find(t);
        if (it != mapManager.getTeams().end()) {
            lbl->setText("$" + std::to_string(it->second.money));
        }
    }
    
    m_panels.undoBtn->setEnabled(!mapManager.isUndoStackEmpty() && !mapManager.isAnyUnitActing());
    
    bool hasNext = false;
    for (auto &u : mapManager.getUnits()) {
        if (!u->isDead() && u->getTeam() == Team::Ally && !u->hasActed()) {
            hasNext = true;
            break;
        }
    }
    m_panels.nextUnitBtn->setEnabled(hasNext && !mapManager.isAnyUnitActing());

    if (mapManager.getCurrentTeam() == Team::Enemy) {
        m_panels.endTurnBtn->setText("Enemy Turn");
        m_panels.endTurnBtn->setEnabled(false);
    } else {
        m_panels.endTurnBtn->setText("End Turn >>");
        m_panels.endTurnBtn->setEnabled(!mapManager.isAnyUnitActing());
    }
}

void GameUIManager::draw() {
    m_gui.draw();
}
