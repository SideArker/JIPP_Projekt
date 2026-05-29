#include "effects/Effects.hpp"
#include "AnimationManager.hpp"
#include "SoundManager.hpp"


void registerEffects() {
	registerHitEffect();
	registerExplosionEffect();

    SoundSet effectSounds;
    effectSounds
        .addSound("explosion", "Art/Sound/explosion.wav")
        .addSound("hit",       "Art/Sound/hit_effect.wav");
    SoundManager::registerSet("Effects", std::move(effectSounds));
}

void registerHitEffect() {
    AnimationSet set;
    set.addClip("hit", AnimationClip::fromRow({0, 0}, {32, 32}, 5, 0.1f, false));
    AnimationManager::registerSet("hitEffect", std::move(set));
}

void registerExplosionEffect() {
    AnimationSet set;
    set.addClip("explode", AnimationClip::fromRow({0, 0}, {32, 64}, 10, 0.1f, false));
    AnimationManager::registerSet("explosion", std::move(set));
}