#include "AnimationManager.hpp"

std::unordered_map<std::string, AnimationSet> AnimationManager::sets;

void AnimationState::play(const std::string& name, const AnimationSet& set, std::function<void()> onFinished) {
    // Allow non-looping clips to restart; skip only if the same looping clip is already playing
    if (name == currentClipName && (currentClip && currentClip->loop)) return;
    const AnimationClip* clip = set.getClip(name);
    if (!clip) return;
    currentClipName = name;
    currentClip = clip;
    currentFrame = 0;
    elapsed = 0.0f;
    m_finished = false;
    m_onFinished = std::move(onFinished);
}

void AnimationState::update(float deltaTime) {
    if (!currentClip || currentClip->frames.empty()) return;
    elapsed += deltaTime;
    while (elapsed >= currentClip->frameTime) {
        elapsed -= currentClip->frameTime;
        ++currentFrame;
        if (currentFrame >= static_cast<int>(currentClip->frames.size())) {
            if (currentClip->loop) {
                currentFrame = 0;
            } else {
                currentFrame = static_cast<int>(currentClip->frames.size()) - 1;
                m_finished = true;
                if (m_onFinished) {
                    // Move out before calling to prevent re-entry if callback calls play()
                    auto cb = std::move(m_onFinished);
                    cb();
                }
                return;
            }
        }
    }
}

sf::IntRect AnimationState::getCurrentRect() const {
    if (!currentClip || currentClip->frames.empty())
        return sf::IntRect({ 0, 0 }, { 32, 32 });
    return currentClip->frames[currentFrame];
}

bool AnimationState::shouldFlipX() const {
    return currentClip && currentClip->flipX;
}

void AnimationManager::registerSet(const std::string& unitType, AnimationSet set) {
    sets.emplace(unitType, std::move(set));
}

const AnimationSet* AnimationManager::getSet(const std::string& unitType) {
    auto it = sets.find(unitType);
    return it != sets.end() ? &it->second : nullptr;
}
