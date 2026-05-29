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
        
    UnitRegistry::registerType("Soldier", [&soldierAnim](Team team) {
        return std::make_shared<Unit>(
            "Soldier",
            "Art/Units/Soldier/soldier_walk.png",
            "Art/Units/Soldier/soldier_walk_mask.png",
            soldierAnim, team, 20, 5, 5
        );
        });

}