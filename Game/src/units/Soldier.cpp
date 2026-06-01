#include "units/Soldier.hpp"
#include "AnimationManager.hpp"
#include "SoundManager.hpp"
#include "UnitRegistry.hpp"
#include "Unit.hpp"
#include <SFML/Graphics.hpp>
#include <memory>

void registerSoldier() {
    AnimationSet soldierAnimSet;
    soldierAnimSet
        .addDirectionalClips("walk",
            AnimationClip::fromRow({ 0,  0 }, { 32, 32 }, 4, 0.1f, false, true), // Soldier is facing right
            AnimationClip::fromRow({ 0, 32 }, { 32, 32 }, 4, 0.1f, false, false),
            AnimationClip::fromRow({ 0, 64 }, { 32, 32 }, 4, 0.1f, false, false));

    AnimationManager::registerSet("Soldier", std::move(soldierAnimSet));


    SoundSet soldierSounds;
    soldierSounds
        .addSound("shoot", "Art/Sound/SoldierAttack.wav");
    SoundManager::registerSet("Soldier", std::move(soldierSounds));

    const AnimationSet& soldierAnim = *AnimationManager::getSet("Soldier");

    UnitData soldierData;
    soldierData.maxHealth = 10;
    soldierData.damage = 5;
    soldierData.moveSpeed = 4;
    soldierData.minAttackRange = 0;
    soldierData.maxAttackRange = 1;
    soldierData.attackDamageDelay = 0.75f;
    soldierData.movementCategory = MovementCategory::Infantry;
    soldierData.deathEffectSet = "explosion";
    soldierData.deathEffectClip  = "explode";
    soldierData.deathEffectTexturePath = "Art/Effects/Explosion.png";
    soldierData.deathSoundSet = "effects";
    soldierData.deathSoundName = "explosion";

    UnitRegistry::registerType("Soldier", soldierData, [&soldierAnim, soldierData](Team team) {
        return std::make_shared<Unit>(
            "Soldier",
            "Art/Units/Soldier/soldier_walk.png",
            "Art/Units/Soldier/soldier_walk_mask.png",
            soldierAnim, team, soldierData
        );
        });

}