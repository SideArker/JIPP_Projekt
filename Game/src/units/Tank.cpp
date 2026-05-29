#include "units/Tank.hpp"
#include "AnimationManager.hpp"
#include "SoundManager.hpp"
#include "UnitRegistry.hpp"
#include "Unit.hpp"
#include <SFML/Graphics.hpp>
#include <memory>

void registerTank() {
    AnimationSet tankAnimSet;
    tankAnimSet
        .addClip("move_left",  AnimationClip::fromRow({0,  0}, {32, 32}))
        .addClip("move_right", AnimationClip::fromRow({0,  0}, {32, 32}, 1, 0.1f, true, true))
        .addClip("move_down",  AnimationClip::fromRow({0, 32}, {32, 32}))
        .addClip("move_up",    AnimationClip::fromRow({0, 64}, {32, 32}))
        .addClip("shoot_left", AnimationClip::fromRow({0, 0}, {32, 32}, 5, 0.1f, false, false))
        .addClip("shoot_right", AnimationClip::fromRow({0, 0}, {32, 32}, 5, 0.1f, false, true))
        .addClip("shoot_down", AnimationClip::fromRow({0, 32}, {32, 32}, 5, 0.1f, false, false))
        .addClip("shoot_up", AnimationClip::fromRow({0, 64}, {32, 32}, 5, 0.1f, false, false));
    AnimationManager::registerSet("Tank", std::move(tankAnimSet));

    SoundSet tankSounds;
    tankSounds
        .addSound("shoot", "Art/Sound/tank_shoot.wav")
        .addSound("hit",   "Art/Sound/tank_hit.wav");
    SoundManager::registerSet("Tank", std::move(tankSounds));

    const AnimationSet& tankAnim = *AnimationManager::getSet("Tank");

    UnitRegistry::registerType("Tank", [&tankAnim](Team team) {
        return std::make_shared<Unit>(
            "Tank",
            "Art/Units/Tank/tank.png",
            "Art/Units/Tank/tank_mask.png",
            tankAnim, team, 20, 5, 5
        );
    });
}

