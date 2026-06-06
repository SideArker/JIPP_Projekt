#include "GameContent.hpp"
#include "AnimationManager.hpp"
#include "MapManager.hpp"
#include "SoundManager.hpp"
#include "TeamRegistry.hpp"
#include "Unit.hpp"
#include "buildings/Buildings.hpp"
#include "effects/Effects.hpp"
#include "units/Barricade.hpp"
#include "units/Destroyer.hpp"
#include "units/MissileTank.hpp"
#include "units/Plane.hpp"
#include "units/Soldier.hpp"
#include "units/Tank.hpp"
#include "units/Turret.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>

void GameContent::init() {
  TeamRegistry::setColor(Team::Ally, sf::Color(50, 255, 50));
  TeamRegistry::setColor(Team::Enemy, sf::Color(255, 7, 58));
  TeamRegistry::setColor(Team::Neutral, sf::Color(255, 255, 255));

  registerTank();
  registerSoldier();
  registerMissileTank();
  registerDestroyer();
  registerPlane();
  registerTurret();
  registerBarricade();

  registerBuildings();
  registerEffects();

  SoundManager::registerMusic("MainMenu", "Art/Sound/MainMenu.wav");
  SoundManager::registerMusic("EnemyTurn", "Art/Sound/EnemyTheme.wav");
  SoundManager::registerMusic("AllyTurn", "Art/Sound/AllyTheme.wav");

  SoundSet gameSound;
  gameSound.addSound("Victory", "Art/Sound/Victory.wav")
      .addSound("Defeat", "Art/Sound/Defeat.wav")
      .addSound("CaptureBounce", "Art/Sound/capture_sound.wav");
  SoundManager::registerSet("GameSound", gameSound);
  SoundManager::setMusicVolume(20.f);
}

void GameContent::configure(MapManager &mapManager) {
  mapManager.setWalkOverlayPath("Art/Effects/Map_Walk_Overlay.png");
  mapManager.setMoveArrowPath("Art/Effects/map_moveArrow.png");
  mapManager.setIconsPath("Art/Effects/Icons.png");
  mapManager.setEnemyOverlayPath("Art/Effects/map_enemy_overlay.png");
  mapManager.setFriendlyOverlayPath("Art/Effects/Unit_Overlay_Friendly.png");
  mapManager.setHitEffect("hitEffect", "hit", "Art/Effects/hitEffect.png");
  mapManager.setTeamCaptureEffect("Art/Effects/TeamCapture.png",
                                  "Art/Effects/TeamCapture_mask.png");
  mapManager.setUnitRenderCallback([&mapManager](sf::RenderTarget &target,
                                                 const Unit &unit,
                                                 bool anyActing) {
    static sf::Texture overlayFriendly, overlayEnemy, healthTex;
    static bool loaded = false;
    if (!loaded) {
      (void)overlayFriendly.loadFromFile(
          "Art/Effects/Unit_Overlay_Friendly.png");
      (void)overlayEnemy.loadFromFile("Art/Effects/Unit_Overlay_Enemy.png");
      (void)healthTex.loadFromFile("Art/Effects/Unit_Health.png");
      loaded = true;
    }
    auto currentTd = mapManager.getTeamData(mapManager.getCurrentTeam());
    bool isCurrentPlayer = currentTd && !currentTd->isAi;
    if (!anyActing && isCurrentPlayer) {
      const bool isInteractable = unit.getIsInteractable();
      const bool showFriendlyOverlay =
          unit.getTeam() == mapManager.getCurrentTeam() && !unit.hasActed() &&
          isInteractable;
      const bool showEnemyOverlay =
          unit.getTeam() != mapManager.getCurrentTeam() || !isInteractable;
      if (showFriendlyOverlay || showEnemyOverlay) {
        const sf::Texture &overlayTex =
            showEnemyOverlay ? overlayEnemy : overlayFriendly;
        sf::Sprite overlaySprite(overlayTex);
        overlaySprite.setPosition(unit.getPosition());
        target.draw(overlaySprite);
      }
    }
    if (unit.getHealth() < unit.getMaxHealth()) {
      float ratio = static_cast<float>(unit.getHealth()) /
                    static_cast<float>(unit.getMaxHealth());
      int frame = std::clamp(static_cast<int>((1.0f - ratio) * 13.0f), 0, 12);
      sf::Sprite healthSprite(healthTex);
      healthSprite.setTextureRect(sf::IntRect({frame * 32, 0}, {32, 32}));
      healthSprite.setPosition(unit.getPosition());
      target.draw(healthSprite);
    }
  });
}