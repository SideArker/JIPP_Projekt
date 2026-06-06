#include "SettingsManager.hpp"
#include <fstream>
#include <sstream>

Settings SettingsManager::load(const std::string& path) {
    Settings s;
    std::ifstream file(path);
    if (!file.is_open()) return s;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            std::string val;
            if (std::getline(iss, val)) {
                if (key == "fullscreen") s.fullscreen = (val == "1");
                else if (key == "musicVolume") s.musicVolume = std::stoi(val);
                else if (key == "soundVolume") s.soundVolume = std::stoi(val);
            }
        }
    }
    return s;
}

void SettingsManager::save(const Settings& settings, const std::string& path) {
    std::ofstream file(path);
    if (file.is_open()) {
        file << "fullscreen=" << (settings.fullscreen ? "1" : "0") << "\n";
        file << "musicVolume=" << settings.musicVolume << "\n";
        file << "soundVolume=" << settings.soundVolume << "\n";
    }
}
