#include "units/Tank.hpp"
#include "AnimationManager.hpp"
#include "SoundManager.hpp"
#include "Unit.hpp"
#include "UnitRegistry.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

void registerTank() {

  ////////////////// VARIABLES
  static const std::string UNIT_NAME = "Tank";
  static const std::string ART_PATH = "Art/Units/Tank/tank_idle.png";
  static const std::string MASK_PATH = "Art/Units/Tank/tank_idle_mask.png";
  static const std::vector<UnitFlag> FLAGS = {};

  AnimationSet unitAnimSet;
  unitAnimSet
      .addDirectionalClips(
          "idle", AnimationClip::fromRow({0, 0}, {32, 32}, 2, 1.f, true, false),
          AnimationClip::fromRow({0, 32}, {32, 32}, 2, 1.f, true, false),
          AnimationClip::fromRow({0, 64}, {32, 32}, 2, 1.f, true, false))

      .addDirectionalClips(
          "walk", AnimationClip::fromRow({0, 0}, {32, 32}, 2, 1.f, true, false),
          AnimationClip::fromRow({0, 32}, {32, 32}, 2, 1.f, true, false),
          AnimationClip::fromRow({0, 64}, {32, 32}, 2, 1.f, true, false))

      .addDirectionalClips(
          "shoot",
          AnimationClip::fromRow({0, 0}, {32, 32}, 5, 0.1f, false, false),
          AnimationClip::fromRow({0, 32}, {32, 32}, 5, 0.1f, false, false),
          AnimationClip::fromRow({0, 64}, {32, 32}, 5, 0.1f, false, false), {},
          "Art/Units/Tank/tank_shoot.png",
          "Art/Units/Tank/tank_shoot_mask.png");

  AnimationManager::registerSet(UNIT_NAME, std::move(unitAnimSet));

  SoundSet unitSounds;
  unitSounds.addSound("shoot", "Art/Sound/TankAttack.wav");
  SoundManager::registerSet(UNIT_NAME, std::move(unitSounds));

  UnitData data;
  data.maxHealth = 40;
  data.damage = 15;
  data.moveSpeed = 5;
  data.minAttackRange = 0;
  data.maxAttackRange = 1;
  data.attackDamageDelay = 0.75f;
  data.movementCategory = MovementCategory::Ground;
  data.deathEffectSet = "explosion";
  data.deathEffectClip = "explode";
  data.deathEffectTexturePath = "Art/Effects/Explosion.png";
  data.deathSoundSet = "effects";
  data.deathSoundName = "explosion";

  const AnimationSet &unitAnim = *AnimationManager::getSet(UNIT_NAME);

  UnitRegistry::registerType(UNIT_NAME, data, [&unitAnim, data](Team team) {
    auto unit = std::make_shared<Unit>(UNIT_NAME, ART_PATH, MASK_PATH, unitAnim,
                                       team, data);
    for (const auto &flag : FLAGS) {
      unit->addFlag(flag);
    }
    return unit;
  });
}