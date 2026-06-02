#include "SoundManager.hpp"

std::unordered_map<std::string, SoundSet> SoundManager::s_sets;
std::unordered_map<std::string, std::string> SoundManager::s_musicPaths;
std::unique_ptr<sf::Music> SoundManager::s_music;
std::string SoundManager::s_currentTrack;
std::list<sf::Sound> SoundManager::s_activeSounds;
float SoundManager::s_musicVolume = 100.f;
float SoundManager::s_sfxVolume   = 100.f;


SoundSet& SoundSet::addSound(const std::string& name, const std::string& filePath) {
    sf::SoundBuffer buffer;
    if (buffer.loadFromFile(filePath))
        m_buffers.emplace(name, std::move(buffer));
    return *this;
}

const sf::SoundBuffer* SoundSet::getBuffer(const std::string& name) const {
    auto it = m_buffers.find(name);
    return it != m_buffers.end() ? &it->second : nullptr;
}



// Sets
void SoundManager::registerSet(const std::string& typeName, SoundSet set) {
    s_sets.emplace(typeName, std::move(set));
}

const SoundSet* SoundManager::getSet(const std::string& typeName) {
    auto it = s_sets.find(typeName);
    return it != s_sets.end() ? &it->second : nullptr;
}


void SoundManager::play(const std::string& typeName, const std::string& soundName) {
    s_activeSounds.remove_if([](const sf::Sound& s) {
        return s.getStatus() == sf::SoundSource::Status::Stopped;
    });

    if (s_activeSounds.size() >= 16) return;

    const SoundSet* set = getSet(typeName);
    if (!set) return;

    const sf::SoundBuffer* buf = set->getBuffer(soundName);
    if (!buf) return;

    s_activeSounds.emplace_back(*buf);
    s_activeSounds.back().setVolume(s_sfxVolume);
    s_activeSounds.back().play();
}

// Music
void SoundManager::registerMusic(const std::string& trackName, const std::string& filePath) {
    s_musicPaths.emplace(trackName, filePath);
}

void SoundManager::playMusic(const std::string& trackName) {
    if (trackName == s_currentTrack) return;

    auto it = s_musicPaths.find(trackName);
    if (it == s_musicPaths.end()) return;

    if (s_music)
        s_music->stop();

    auto music = std::make_unique<sf::Music>();
    if (!music->openFromFile(it->second)) return;

    s_music = std::move(music);
    s_music->setLooping(true);
    s_music->setVolume(s_musicVolume);
    s_music->play();
    s_currentTrack = trackName;
}

void SoundManager::stopMusic() {
    if (s_music)
        s_music->stop();
    s_currentTrack.clear();
}

void SoundManager::shutdown() {
    stopMusic();

    for (auto& sound : s_activeSounds)
        sound.stop();
    s_activeSounds.clear();

    s_music.reset();
    s_currentTrack.clear();
    s_musicPaths.clear();
    s_sets.clear();
}

void SoundManager::setMusicVolume(float volume) {
    s_musicVolume = volume;
    if (s_music)
        s_music->setVolume(volume);
}

void SoundManager::setSFXVolume(float volume) {
    s_sfxVolume = volume;
    for (auto& sound : s_activeSounds)
        sound.setVolume(volume);
}
