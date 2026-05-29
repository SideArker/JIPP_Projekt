#include "effects/Effects.hpp"
#include "AnimationManager.hpp"


void registerEffects() {
	registerHitEffect();
	registerExplosionEffect();
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