#include "units/Turret.hpp"
#include "AnimationManager.hpp"
#include "SoundManager.hpp"
#include "Unit.hpp"
#include "UnitRegistry.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

void registerTurret() {

  ////////////////// VARIABLES
  static const std::string UNIT_NAME = "Turret";
  static const std::string ART_PATH = "Art/Units/Turret/turret.png";
  static const std::string MASK_PATH = "Art/Units/Turret/turret_mask.png";
  static const std::vector<UnitFlag> FLAGS = {
      UnitFlag::CanAttackGround, UnitFlag::CanAttackInfantry,
      UnitFlag::CanAttackFlying, UnitFlag::CanAttackNaval};

  AnimationSet unitAnimSet;
  unitAnimSet
      .addDirectionalClips(
          "idle", AnimationClip::fromRow({0, 0}, {32, 32}, 1, 1.f, true, false),
          AnimationClip::fromRow({0, 32}, {32, 32}, 1, 1.f, true, false),
          AnimationClip::fromRow({0, 64}, {32, 32}, 1, 1.f, true, false))
      .addDirectionalClips(
          "shoot",
          AnimationClip::fromRow({0, 0}, {32, 32}, 5, 0.1f, false, false),
          AnimationClip::fromRow({0, 32}, {32, 32}, 5, 0.1f, false, false),
          AnimationClip::fromRow({0, 64}, {32, 32}, 5, 0.1f, false, false));

  AnimationManager::registerSet(UNIT_NAME, std::move(unitAnimSet));

  SoundSet unitSounds;
  unitSounds.addSound("shoot", "Art/Sound/TankAttack.wav");
  SoundManager::registerSet(UNIT_NAME, std::move(unitSounds));

  UnitData data;
  data.maxHealth = 40;
  data.damage = 15;
  data.moveSpeed = 0;
  data.minAttackRange = 0;
  data.maxAttackRange = 3;
  data.cost = 0;
  data.hitEffectDelay = 0.0f;
  data.attackDamageDelay = 0.75f;
  data.movementCategory = MovementCategory::None;
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