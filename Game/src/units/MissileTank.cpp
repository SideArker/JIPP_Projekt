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
    static const std::vector<UnitFlag> FLAGS = {};

    AnimationSet unitAnimSet;
    unitAnimSet
        .addDirectionalClips("idle",
            AnimationClip::fromRow({ 0,  0 }, { 32, 32 }, 2, 1.f, true, true),
            AnimationClip::fromRow({ 0, 64 }, { 32, 32 }, 2, 1.f, true, false), // Up
            AnimationClip::fromRow({ 0, 32 }, { 32, 32 }, 2, 1.f, true, false)) // Down

        .addDirectionalClips("walk",
            AnimationClip::fromRow({ 0,  0 }, { 32, 32 }, 3, 0.1f, false, true),
            AnimationClip::fromRow({ 0, 64 }, { 32, 32 }, 3, 0.1f, false, false),
            AnimationClip::fromRow({ 0, 32 }, { 32, 32 }, 3, 0.1f, false, false),
            {}, "Art/Units/MissileTank/missiletank_move.png", "Art/Units/MissileTank/missiletank_move_mask.png")

        .addDirectionalClips("shoot",
                AnimationClip::fromRow({ 0,  0 }, { 32, 32 }, 5, 0.1f, false, true),
                AnimationClip::fromRow({ 0, 64 }, { 32, 32 }, 5, 0.1f, false, false),
                AnimationClip::fromRow({ 0, 32 }, { 32, 32 }, 5, 0.1f, false, false),
                {}, "Art/Units/MissileTank/missiletank_shoot.png", "Art/Units/MissileTank/missiletank_shoot_mask.png");

    AnimationManager::registerSet(UNIT_NAME, std::move(unitAnimSet));

    SoundSet unitSounds;
    unitSounds
        .addSound("shoot", "Art/Sound/missiletank_shoot.wav")
        .addSound("hit", "Art/Sound/missiletank_hit.wav");
    SoundManager::registerSet(UNIT_NAME, std::move(unitSounds));

    UnitData data;
    data.maxHealth = 10;
    data.damage = 10;
    data.moveSpeed = 2;
    data.minAttackRange = 2;
    data.maxAttackRange = 6;
    data.hitEffectDelay = 1.1f;
    data.attackDamageDelay = 0.9f;
    data.movementCategory = MovementCategory::Ground;
    data.deathEffectSet = "explosion";
    data.deathEffectClip = "explode";
    data.deathEffectTexturePath = "Art/Effects/Explosion.png";
    data.deathSoundSet = "effects";
    data.deathSoundName = "explosion";

    const AnimationSet& unitAnim = *AnimationManager::getSet(UNIT_NAME);

    UnitRegistry::registerType(UNIT_NAME, data, [&unitAnim, data](Team team) {
        auto unit = std::make_shared<Unit>(UNIT_NAME, ART_PATH, MASK_PATH, unitAnim, team, data);
        for (const auto& flag : FLAGS) {
            unit->addFlag(flag);
        }
        return unit;
        });

}