#include "units/Soldier.hpp"
#include "AnimationManager.hpp"
#include "SoundManager.hpp"
#include "Unit.hpp"
#include "UnitRegistry.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

void registerSoldier() {

  ////////////////// VARIABLES
  static const std::string UNIT_NAME = "Soldier";
  static const std::string ART_PATH = "Art/Units/Soldier/soldier_walk.png";
  static const std::string MASK_PATH =
      "Art/Units/Soldier/soldier_walk_mask.png";
  static const std::vector<UnitFlag> FLAGS = {UnitFlag::Capture,
                                              UnitFlag::CanAttackGround,
                                              UnitFlag::CanAttackInfantry};

  AnimationSet unitAnimSet;
  unitAnimSet.addDirectionalClips(
      "walk",
      AnimationClip::fromRow({0, 0}, {32, 32}, 4, 0.1f, false,
                             true), // Soldier is facing right
      AnimationClip::fromRow({0, 32}, {32, 32}, 4, 0.1f, false, false),
      AnimationClip::fromRow({0, 64}, {32, 32}, 4, 0.1f, false, false));

  AnimationManager::registerSet(UNIT_NAME, std::move(unitAnimSet));

  SoundSet unitSounds;
  unitSounds.addSound("shoot", "Art/Sound/SoldierAttack.wav");
  SoundManager::registerSet(UNIT_NAME, std::move(unitSounds));

  UnitData data;
  data.maxHealth = 30;
  data.damage = 10;
  data.moveSpeed = 3;
  data.minAttackRange = 0;
  data.maxAttackRange = 1;
  data.cost = 100;
  data.hitEffectDelay = 0.0f;
  data.attackDamageDelay = 0.75f;
  data.movementCategory = MovementCategory::Infantry;
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