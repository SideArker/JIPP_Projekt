#include "units/MissileTank.hpp"
#include "AnimationManager.hpp"
#include "SoundManager.hpp"
#include "UnitRegistry.hpp"
#include "Unit.hpp"
#include <SFML/Graphics.hpp>
#include <memory>

void registerMissileTank() {

////////////////// VARIABLES
    static const std::string UNIT_NAME = "MissileTank";
	static const std::string ART_PATH = "Art/Units/MissileTank/missiletank.png";
	static const std::string MASK_PATH = "Art/Units/MissileTank/missiletank_mask.png";


    AnimationSet unitAnimSet;
    unitAnimSet
        .addDirectionalClips("walk",
            AnimationClip::fromRow({ 0,  0 }, { 32, 32 }, 4, 0.1f, false, true),
            AnimationClip::fromRow({ 0, 32 }, { 32, 32 }, 4, 0.1f, false, false),
            AnimationClip::fromRow({ 0, 64 }, { 32, 32 }, 4, 0.1f, false, false));

    AnimationManager::registerSet(UNIT_NAME, std::move(unitAnimSet));

    SoundSet unitSounds;
    unitSounds
        .addSound("shoot", "Art/Sound/missiletank_shoot.wav")
        .addSound("hit", "Art/Sound/missiletank_hit.wav");
    SoundManager::registerSet(UNIT_NAME, std::move(unitSounds));

    UnitData data;
    data.maxHealth = 10;
    data.damage = 3;
    data.moveSpeed = 3;
    data.minAttackRange = 2;
    data.maxAttackRange = 6;
    data.hitEffectDelay = 0.9f;

    const AnimationSet& unitAnim = *AnimationManager::getSet(UNIT_NAME);

    UnitRegistry::registerType(UNIT_NAME, data, [&unitAnim, data](Team team) {
        return std::make_shared<Unit>(UNIT_NAME, ART_PATH, MASK_PATH, unitAnim, team, data);
        });

}