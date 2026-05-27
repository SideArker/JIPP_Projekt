#include "units/Tank.hpp"
#include "AnimationManager.hpp"
#include "UnitRegistry.hpp"
#include "Unit.hpp"
#include <SFML/Graphics.hpp>
#include <memory>

void registerTank() {
    AnimationSet tankAnimSet;
    tankAnimSet
        .addClip("move_left",  AnimationClip{ { sf::IntRect({0,  0}, {32, 32}) }, 0.1f, true, false })
        .addClip("move_right", AnimationClip{ { sf::IntRect({0,  0}, {32, 32}) }, 0.1f, true, true  })
        .addClip("move_down",  AnimationClip{ { sf::IntRect({0, 32}, {32, 32}) }, 0.1f, true, false })
        .addClip("move_up",    AnimationClip{ { sf::IntRect({0, 64}, {32, 32}) }, 0.1f, true, false });
    AnimationManager::registerSet("Tank", std::move(tankAnimSet));

    const AnimationSet& tankAnim = *AnimationManager::getSet("Tank");

    UnitRegistry::registerType("Tank", [&tankAnim](Team team) {
        return std::make_shared<Unit>(
            "Tank",
            "Art/tank-shoot-grayscale.png",
            "Art/tank-shoot-grayscale-mask.png",
            tankAnim, team, 20, 5, 5
        );
    });
}

