#include "units/Barricade.hpp"
#include "AnimationManager.hpp"
#include "SoundManager.hpp"
#include "Unit.hpp"
#include "UnitRegistry.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

void registerBarricade() {

  ////////////////// VARIABLES
  static const std::string UNIT_NAME = "Barricade";
  static const std::string ART_PATH = "Art/Units/Barricade/barricade.png";
  static const std::string MASK_PATH = "Art/Units/Barricade/barricade_mask.png";
  static const std::vector<UnitFlag> FLAGS = {};

  AnimationSet unitAnimSet;
  unitAnimSet
      .addDirectionalClips(
          "idle", AnimationClip::fromRow({0, 0}, {32, 32}, 1, 1.f, true, false),
          AnimationClip::fromRow({0, 0}, {32, 32}, 1, 1.f, true, false),
          AnimationClip::fromRow({0, 0}, {32, 32}, 1, 1.f, true, false))
      .addDirectionalClips(
          "attack",
          AnimationClip::fromRow({0, 0}, {32, 32}, 1, 0.1f, false, false),
          AnimationClip::fromRow({0, 0}, {32, 32}, 1, 0.1f, false, false),
          AnimationClip::fromRow({0, 0}, {32, 32}, 1, 0.1f, false, false));

  AnimationManager::registerSet(UNIT_NAME, std::move(unitAnimSet));

  UnitData data;
  data.maxHealth = 60;
  data.damage = 0;
  data.moveSpeed = 0;
  data.minAttackRange = 0;
  data.maxAttackRange = 0;
  data.cost = 0;
  data.hitEffectDelay = 0.0f;
  data.attackDamageDelay = 0.75f;
  data.movementCategory = MovementCategory::None;
  data.deathEffectSet = "explosion";
  data.deathEffectClip = "explode";
  data.deathEffectTexturePath = "Art/Effects/Explosion.png";
  data.deathSoundSet = "effects";
  data.deathSoundName = "explosion";
  data.isInteractable = false;

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