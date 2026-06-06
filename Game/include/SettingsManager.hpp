#pragma once

#include <string>

struct Settings {
    bool fullscreen = false;
    int musicVolume = 20;
    int soundVolume = 100;
};

class SettingsManager {
public:
    static Settings load(const std::string& path = "settings.txt");
    static void save(const Settings& settings, const std::string& path = "settings.txt");
};
