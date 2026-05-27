#pragma once

#include "EngineAPI.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

struct AnimationClip {
	// Which pixels get cut out of the texture for each frame
    std::vector<sf::IntRect> frames;
    float frameTime = 0.1f;
    bool loop = true;
    bool flipX = false;
};

struct AnimationSet {
    std::unordered_map<std::string, AnimationClip> clips;

    AnimationSet& addClip(const std::string& name, AnimationClip clip) {
        clips.emplace(name, std::move(clip));
        return *this;
    }

    const AnimationClip* getClip(const std::string& name) const {
        auto it = clips.find(name);
        return it != clips.end() ? &it->second : nullptr;
    }
};

class ENGINE_API AnimationState {
private:
    std::string currentClipName;
    const AnimationClip* currentClip = nullptr;
    int currentFrame = 0;
    float elapsed = 0.0f;
    bool m_finished = false;
    std::function<void()> m_onFinished;
public:
    void play(const std::string& name, const AnimationSet& set, std::function<void()> onFinished = {});
    void update(float deltaTime);
    sf::IntRect getCurrentRect() const;
    bool shouldFlipX() const;
    bool isFinished() const { return m_finished; }
};

class ENGINE_API AnimationManager {
    static std::unordered_map<std::string, AnimationSet> sets;
public:
    static void registerSet(const std::string& unitType, AnimationSet set);
    static const AnimationSet* getSet(const std::string& unitType);
};
