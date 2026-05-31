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
        .addDirectionalClips("idle",
            AnimationClip::fromRow({0,  0}, {32, 32}, 2, 1.f, true, false),
            AnimationClip::fromRow({0, 32}, {32, 32}, 2, 1.f, true, false),
            AnimationClip::fromRow({0, 64}, {32, 32}, 2, 1.f, true, false))

        .addDirectionalClips("walk",
            AnimationClip::fromRow({ 0,  0 }, { 32, 32 }, 2, 1.f, true, false),
            AnimationClip::fromRow({ 0, 32 }, { 32, 32 }, 2, 1.f, true, false),
            AnimationClip::fromRow({ 0, 64 }, { 32, 32 }, 2, 1.f, true, false))

        .addDirectionalClips("shoot",
            AnimationClip::fromRow({0,  0}, {32, 32}, 5, 0.1f, false, false),
            AnimationClip::fromRow({0, 32}, {32, 32}, 5, 0.1f, false, false),
            AnimationClip::fromRow({ 0, 64 }, { 32, 32 }, 5, 0.1f, false, false),
            {}, "Art/Units/Tank/tank_shoot.png", "Art/Units/Tank/tank_shoot_mask.png");

    AnimationManager::registerSet("Tank", std::move(tankAnimSet));

    SoundSet tankSounds;
    tankSounds
        .addSound("shoot", "Art/Sound/TankAttack.wav");
    SoundManager::registerSet("Tank", std::move(tankSounds));

    const AnimationSet& tankAnim = *AnimationManager::getSet("Tank");

    UnitData tankData;
    tankData.maxHealth = 20;
    tankData.damage = 5;
    tankData.moveSpeed = 5;
    tankData.minAttackRange = 2;
    tankData.maxAttackRange = 6;

    UnitRegistry::registerType("Tank", tankData, [&tankAnim, tankData](Team team) {
        return std::make_shared<Unit>(
            "Tank",
            "Art/Units/Tank/tank_idle.png",
            "Art/Units/Tank/tank_idle_mask.png",
            tankAnim, team, tankData
        );
    });
}

