#pragma once

#include "EngineAPI.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

struct ENGINE_API AnimationClip {
    std::vector<sf::IntRect> frames;
    float frameTime = 0.1f;
    bool loop = true;
    bool flipX = false;
    std::string texturePath; // empty = use the unit's base texture
    std::string maskPath;

    /// Builds a clip by reading 'count' tiles left-to-right from a spritesheet row.
    /// @param origin     Top-left pixel of the first tile (x, y).
    /// @param tileSize   Width and height of each tile in pixels (w, h).
    /// @param count      Number of frames to extract (default 1 for single-frame clips).
    /// @param frameTime  Seconds each frame is displayed.
    /// @param loop       Whether the clip loops back to frame 0 after the last frame.
    /// @param flipX      Whether to mirror the sprite horizontally on render.
    static AnimationClip fromRow(sf::Vector2i origin, sf::Vector2i tileSize, int count = 1, float frameTime = 0.1f, bool loop = true, bool flipX = false);

    /// Builds a clip by reading 'count' tiles top-to-bottom from a spritesheet column.
    static AnimationClip fromColumn(sf::Vector2i origin, sf::Vector2i tileSize, int count = 1, float frameTime = 0.1f, bool loop = true, bool flipX = false);
};

struct AnimationSet {
    std::unordered_map<std::string, AnimationClip> clips;

    AnimationSet& addClip(const std::string& name, AnimationClip clip) {
        clips.emplace(name, std::move(clip));
        return *this;
    }

    AnimationSet& addDirectionalClips(
                                    const std::string& name, 
                                    AnimationClip leftClip,
                                    AnimationClip downClip, 
                                    AnimationClip upClip, 
                                    AnimationClip rightClip = {},
                                    const std::string& texturePath = {},
                                    const std::string& maskPath = {}) {

        if (rightClip.frames.empty()) {
            rightClip = leftClip;
            rightClip.flipX = !leftClip.flipX;
        }
        if (!texturePath.empty()) {
            leftClip.texturePath  = texturePath;  leftClip.maskPath  = maskPath;
            rightClip.texturePath = texturePath;  rightClip.maskPath = maskPath;
            downClip.texturePath  = texturePath;  downClip.maskPath  = maskPath;
            upClip.texturePath    = texturePath;  upClip.maskPath    = maskPath;
        }
        clips.emplace(name + "_left",  std::move(leftClip));
        clips.emplace(name + "_right", std::move(rightClip));
        clips.emplace(name + "_down",  std::move(downClip));
        clips.emplace(name + "_up",    std::move(upClip));
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
    void resetToFrameZero() {
        currentFrame = 0;
        elapsed = 0.0f;
        m_finished = false;
    }
    sf::IntRect getCurrentRect() const;
    bool shouldFlipX() const;
    bool isFinished() const { return m_finished; }
    const AnimationClip* getCurrentClip() const { return currentClip; }
};

class ENGINE_API AnimationManager {
    static std::unordered_map<std::string, AnimationSet> sets;
public:
    static void registerSet(const std::string& unitType, AnimationSet set);
    static const AnimationSet* getSet(const std::string& unitType);
};
