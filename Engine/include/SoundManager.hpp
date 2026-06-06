#pragma once

#include "EngineAPI.hpp"
#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <list>
#include <memory>

struct ENGINE_API SoundSet {
    SoundSet& addSound(const std::string& name, const std::string& filePath);
    const sf::SoundBuffer* getBuffer(const std::string& name) const;

private:
    std::unordered_map<std::string, sf::SoundBuffer> m_buffers;
};

class ENGINE_API SoundManager {
public:
    static void registerSet(const std::string& typeName, SoundSet set);
    static const SoundSet* getSet(const std::string& typeName);

    static void play(const std::string& typeName, const std::string& soundName);

    static void registerMusic(const std::string& trackName, const std::string& filePath);
    static void playMusic(const std::string& trackName, bool loop = true);
    static void stopMusic();
    static void shutdown();

    static void setMusicVolume(float volume);
    static void setSFXVolume(float volume);

private:
    static std::unordered_map<std::string, SoundSet> s_sets;
    static std::unordered_map<std::string, std::string> s_musicPaths;
    static std::unique_ptr<sf::Music> s_music;
    static std::string s_currentTrack;
    static std::list<sf::Sound> s_activeSounds;
    static float s_musicVolume;
    static float s_sfxVolume;
};
