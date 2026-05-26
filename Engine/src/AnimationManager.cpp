#include "AnimationManager.hpp"

std::unordered_map<std::string, AnimationSet> AnimationManager::sets;

void AnimationState::play(const std::string& name, const AnimationSet& set) {
    if (name == currentClipName) return;
    const AnimationClip* clip = set.getClip(name);
    if (!clip) return;
    currentClipName = name;
    currentClip = clip;
    currentFrame = 0;
    elapsed = 0.0f;
}

void AnimationState::update(float deltaTime) {
    if (!currentClip || currentClip->frames.empty()) return;
    elapsed += deltaTime;
    while (elapsed >= currentClip->frameTime) {
        elapsed -= currentClip->frameTime;
        ++currentFrame;
        if (currentFrame >= static_cast<int>(currentClip->frames.size())) {
            currentFrame = currentClip->loop ? 0 : static_cast<int>(currentClip->frames.size()) - 1;
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
