#include "effects/HitEffect.hpp"
#include "AnimationManager.hpp"

void registerHitEffect() {
    AnimationSet set;
    set.addClip("hit", AnimationClip::fromRow({0, 0}, {32, 32}, 5, 0.1f, false));
    AnimationManager::registerSet("hitEffect", std::move(set));
}
