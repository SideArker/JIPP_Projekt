#include "effects/HitEffect.hpp"
#include "AnimationManager.hpp"
#include <SFML/Graphics.hpp>

void registerHitEffect() {
    // Frames read left-to-right on a single row; adjust rects to match Art/hitEffect.png
    AnimationSet set;
    set.addClip("hit", AnimationClip{
        {
            sf::IntRect({0,  0}, {32, 32}),
            sf::IntRect({32, 0}, {32, 32}),
            sf::IntRect({64, 0}, {32, 32}),
            sf::IntRect({96, 0}, {32, 32}),
            sf::IntRect({128, 0}, {32, 32}),
        },
        0.1f, false, false
    });
    AnimationManager::registerSet("hitEffect", std::move(set));
}
