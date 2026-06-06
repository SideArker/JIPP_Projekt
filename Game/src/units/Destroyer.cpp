#include "units/Destroyer.hpp"
#include "AnimationManager.hpp"
#include "SoundManager.hpp"
#include "Unit.hpp"
#include "UnitRegistry.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

void registerDestroyer() {

  ////////////////// VARIABLES
  static const std::string UNIT_NAME = "Destroyer";
  static const std::string ART_PATH = "Art/Units/Destroyer/destroyer_move.png";
  static const std::string MASK_PATH =
      "Art/Units/Destroyer/destroyer_move_mask.png";
  static const std::vector<UnitFlag> FLAGS = {
      UnitFlag::CanAttackGround, UnitFlag::CanAttackInfantry,
      UnitFlag::CanAttackFlying, UnitFlag::CanAttackNaval};

  AnimationSet unitAnimSet;
  unitAnimSet.addDirectionalClips(
      "idle", AnimationClip::fromRow({0, 0}, {32, 32}, 1, 1.f, true, true),
      AnimationClip::fromRow({0, 32}, {32, 32}, 1, 1.f, true, false),
      AnimationClip::fromRow({0, 64}, {32, 32}, 1, 1.f, true, false));

  unitAnimSet.addDirectionalClips(
      "walk", AnimationClip::fromRow({0, 0}, {32, 32}, 3, 0.1f, false, true),
      AnimationClip::fromRow({0, 32}, {32, 32}, 3, 0.1f, false, false),
      AnimationClip::fromRow({0, 64}, {32, 32}, 3, 0.1f, false, false));

  unitAnimSet.addDirectionalClips(
      "shoot", AnimationClip::fromRow({0, 0}, {32, 32}, 5, 0.1f, false, true),
      AnimationClip::fromRow({0, 32}, {32, 32}, 5, 0.1f, false, false),
      AnimationClip::fromRow({0, 64}, {32, 32}, 5, 0.1f, false, false), {},
      "Art/Units/Destroyer/destroyer_shoot.png",
      "Art/Units/Destroyer/destroyer_shoot_mask.png");

  AnimationManager::registerSet(UNIT_NAME, std::move(unitAnimSet));

  SoundSet unitSounds;
  unitSounds.addSound("shoot", "Art/Sound/destroyer_attack.wav")
      .addSound("ready", "Art/Sound/destroyer_move.wav")
      .addSound("move", "Art/Sound/destroyer_move.wav");
  SoundManager::registerSet(UNIT_NAME, std::move(unitSounds));

  UnitData data;
  data.maxHealth = 40;
  data.damage = 20;
  data.moveSpeed = 4;
  data.minAttackRange = 0;
  data.maxAttackRange = 4;
  data.cost = 400;
  data.hitEffectDelay = 0.0f;
  data.attackDamageDelay = 0.75f;
  data.movementCategory = MovementCategory::Naval;
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