#pragma once

#include <SFML/Graphics.hpp>
#include <string>

enum class MainMenuAction {
    PlayChapter,
    LoadSave,
    Quit
};

struct MainMenuResult {
    MainMenuAction action;
    std::string path;
};

MainMenuResult runMainMenu(sf::RenderWindow& window);
