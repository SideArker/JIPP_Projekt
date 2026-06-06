#include "MainMenuUI.hpp"
#include "RightPanelUI.hpp"
#include "SettingsManager.hpp"
#include "SoundManager.hpp"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <SFML/Window/Context.hpp>
#include <filesystem>
#include <cstdlib>
#include <iostream>

MainMenuResult runMainMenu(sf::RenderWindow& window) {
    SoundManager::playMusic("MainMenu");
    tgui::Gui gui(window);
    gui.setFont("Art/Fonts/joystixMonospace.ttf");

    MainMenuResult result;
    bool done = false;

    auto background = tgui::Panel::create({"100%", "100%"});
    background->getRenderer()->setBackgroundColor(sf::Color(15, 25, 35));
    gui.add(background);

    auto title = tgui::Label::create("JIPP PROJEKT");
    title->setPosition("50%", "15%");
    title->setOrigin(0.5f, 0.5f);
    title->setTextSize(48);
    title->getRenderer()->setTextColor(sf::Color(200, 220, 255));
    gui.add(title);

    auto mainContainer = tgui::Panel::create({"400px", "500px"});
    mainContainer->setPosition("50%", "55%");
    mainContainer->setOrigin(0.5f, 0.5f);
    mainContainer->getRenderer()->setBackgroundColor(sf::Color::Transparent);
    gui.add(mainContainer);

    auto createBtn = [](const std::string &text, float yPos, tgui::Panel::Ptr parent) {
        auto btn = tgui::Button::create(text);
        btn->setSize("100%", "50px");
        btn->setPosition(0, yPos);
        btn->getRenderer()->setBackgroundColor(sf::Color(30, 45, 60));
        btn->getRenderer()->setBackgroundColorHover(sf::Color(50, 75, 100));
        btn->getRenderer()->setBackgroundColorDown(sf::Color(20, 30, 40));
        btn->getRenderer()->setTextColor(sf::Color(200, 220, 255));
        btn->getRenderer()->setBorders(tgui::Borders(1));
        btn->getRenderer()->setBorderColor(sf::Color(60, 85, 115));
        parent->add(btn);
        return btn;
    };

    auto playBtn = createBtn("Play Chapter", 0, mainContainer);
    auto loadBtn = createBtn("Load Game", 60, mainContainer);
    auto editorBtn = createBtn("Map Editor", 120, mainContainer);
    auto settingsBtn = createBtn("Settings", 180, mainContainer);
    auto quitBtn = createBtn("Quit", 240, mainContainer);

    auto chapterPanel = tgui::Panel::create({"400px", "400px"});
    chapterPanel->setPosition("50%", "50%");
    chapterPanel->setOrigin(0.5f, 0.5f);
    chapterPanel->getRenderer()->setBackgroundColor(sf::Color(25, 35, 45, 240));
    chapterPanel->getRenderer()->setBorders(tgui::Borders(2));
    chapterPanel->getRenderer()->setBorderColor(sf::Color(60, 85, 115));
    chapterPanel->setVisible(false);
    gui.add(chapterPanel);

    auto chapterTitle = tgui::Label::create("Select Chapter");
    chapterTitle->setPosition("50%", "10px");
    chapterTitle->setOrigin(0.5f, 0.f);
    chapterTitle->setTextSize(20);
    chapterTitle->getRenderer()->setTextColor(sf::Color::White);
    chapterPanel->add(chapterTitle);

    auto chapterList = tgui::ScrollablePanel::create({"100% - 40px", "100% - 100px"});
    chapterList->setPosition("20px", "50px");
    chapterPanel->add(chapterList);

    auto closeChapterBtn = tgui::Button::create("Back");
    closeChapterBtn->setSize("100px", "30px");
    closeChapterBtn->setPosition("50%", "100% - 40px");
    closeChapterBtn->setOrigin(0.5f, 0.f);
    chapterPanel->add(closeChapterBtn);

    playBtn->onClick([&]() {
        chapterList->removeAllWidgets();
        float y = 0;
        if (std::filesystem::exists("Art/levels")) {
            for (const auto& entry : std::filesystem::directory_iterator("Art/levels")) {
                if (entry.path().extension() == ".map") {
                    std::string filename = entry.path().filename().string();
                    std::string filepath = entry.path().string();
                    
                    auto btn = tgui::Button::create(filename);
                    btn->setSize("100%", "40px");
                    btn->setPosition(0, y);
                    btn->onClick([&, filepath]() {
                        result.action = MainMenuAction::PlayChapter;
                        result.path = filepath;
                        done = true;
                    });
                    chapterList->add(btn);
                    y += 45;
                }
            }
        }
        mainContainer->setVisible(false);
        chapterPanel->setVisible(true);
    });

    closeChapterBtn->onClick([&]() {
        chapterPanel->setVisible(false);
        mainContainer->setVisible(true);
    });

    loadBtn->onClick([&]() {
        auto fileDialog = tgui::FileDialog::create("Load Save Game", "Load");
        fileDialog->setPath(std::filesystem::current_path().string());
        fileDialog->setFileTypeFilters({{"Save Files", {"*.sav"}}, {"All Files", {"*"}}});
        fileDialog->onFileSelect([&](const tgui::String& path) {
            result.action = MainMenuAction::LoadSave;
            result.path = path.toStdString();
            done = true;
        });
        gui.add(fileDialog);
    });

    editorBtn->onClick([]() {
        std::system("start MapEditor.exe");
    });

    GameUIWidgets settingsWidgets;
    auto settingsPanel = buildSettingsPanel(gui, &settingsWidgets);
    
    settingsWidgets.closeSettingsBtn->onClick([&]() {
        settingsPanel->setVisible(false);
        mainContainer->setVisible(true);
    });

    settingsWidgets.saveGameBtn->setVisible(false);
    settingsWidgets.quitBtn->setVisible(false);
    settingsWidgets.closeSettingsBtn->setPosition("50%", "240px");
    settingsWidgets.closeSettingsBtn->setOrigin(0.5f, 0.f);
    settingsWidgets.closeSettingsBtn->setSize("360px", "40px");

    Settings settings = SettingsManager::load();
    settingsWidgets.musicVolSlider->setValue(settings.musicVolume);
    settingsWidgets.soundVolSlider->setValue(settings.soundVolume);
    settingsWidgets.fullscreenCheckbox->setChecked(settings.fullscreen);

    settingsWidgets.musicVolSlider->onValueChange([](float v) { 
        SoundManager::setMusicVolume(v); 
        Settings s = SettingsManager::load();
        s.musicVolume = static_cast<int>(v);
        SettingsManager::save(s);
    });
    settingsWidgets.soundVolSlider->onValueChange([](float v) { 
        SoundManager::setSFXVolume(v); 
        Settings s = SettingsManager::load();
        s.soundVolume = static_cast<int>(v);
        SettingsManager::save(s);
    });
    settingsWidgets.fullscreenCheckbox->onChange([&window, &gui](bool checked) {
        Settings s = SettingsManager::load();
        s.fullscreen = checked;
        SettingsManager::save(s);

        sf::Context context;
        auto style = checked ? sf::State::Fullscreen : sf::State::Windowed;
        window.create(sf::VideoMode({1280, 720}), "Map Renderer", style);
        sf::Image icon;
        if (icon.loadFromFile("Art/ico.png")) {
            window.setIcon({icon.getSize().x, icon.getSize().y}, icon.getPixelsPtr());
        }
        gui.setWindow(window);
    });

    settingsBtn->onClick([&]() {
        mainContainer->setVisible(false);
        settingsPanel->setVisible(true);
    });

    quitBtn->onClick([&]() {
        result.action = MainMenuAction::Quit;
        done = true;
    });

    while (window.isOpen() && !done) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                result.action = MainMenuAction::Quit;
                done = true;
                break;
            }
            gui.handleEvent(*event);
        }

        window.clear();
        gui.draw();
        window.display();
    }

    return result;
}
