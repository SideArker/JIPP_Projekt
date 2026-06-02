#include "GameContent.hpp"
#include "MapManager.hpp"
#include "TeamRegistry.hpp"
#include "AnimationManager.hpp"
#include "SoundManager.hpp"
#include "Unit.hpp"
#include "units/Tank.hpp"
#include "units/Soldier.hpp"
#include "units/MissileTank.hpp"
#include "buildings/Buildings.hpp"
#include "effects/Effects.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>

void GameContent::init() {
    TeamRegistry::setColor(Team::Ally,    sf::Color( 50, 255,  50));
    TeamRegistry::setColor(Team::Enemy,   sf::Color(255,   7,  58));
    TeamRegistry::setColor(Team::Neutral, sf::Color(200, 200, 200));

    registerTank();
    registerSoldier();
	registerMissileTank();
    registerBuildings();
    registerEffects();

    SoundManager::registerMusic("MainMenu",  "Art/Sound/main_menu.ogg");
    SoundManager::registerMusic("EnemyTurn", "Art/Sound/EnemyTheme.wav");
    SoundManager::registerMusic("AllyTurn",  "Art/Sound/AllyTheme.wav");
    SoundManager::setMusicVolume(20.f);

}

void GameContent::configure(MapManager& mapManager) {
    mapManager.setWalkOverlayPath("Art/Effects/Map_Walk_Overlay.png");
    mapManager.setMoveArrowPath("Art/Effects/map_moveArrow.png");
    mapManager.setIconsPath("Art/Effects/Icons.png");
    mapManager.setEnemyOverlayPath("Art/Effects/map_enemy_overlay.png");
    mapManager.setHitEffect("hitEffect", "hit", "Art/Effects/hitEffect.png");
    mapManager.setTeamCaptureEffect("Art/Effects/TeamCapture.png", "Art/Effects/TeamCapture_mask.png");
    mapManager.setUnitRenderCallback([](sf::RenderTarget& target, const Unit& unit, bool anyActing) {
        
        static sf::Texture overlayFriendly, overlayEnemy, healthTex;
        static bool loaded = false;
        if (!loaded) {
            overlayFriendly.loadFromFile("Art/Effects/Unit_Overlay_Friendly.png");
            overlayEnemy.loadFromFile("Art/Effects/Unit_Overlay_Enemy.png");
            healthTex.loadFromFile("Art/Effects/Unit_Health.png");
            loaded = true;
        }
        if (!anyActing) {
            const bool showFriendlyOverlay = unit.getTeam() == Team::Ally && !unit.hasActed();
            const bool showEnemyOverlay = unit.getTeam() == Team::Enemy;
            if (showFriendlyOverlay || showEnemyOverlay) {
                const sf::Texture& overlayTex = showEnemyOverlay ? overlayEnemy : overlayFriendly;
                sf::Sprite overlaySprite(overlayTex);
                overlaySprite.setPosition(unit.getPosition());
                target.draw(overlaySprite);
            }
        }
        if (unit.getHealth() < unit.getMaxHealth()) {
            float ratio = static_cast<float>(unit.getHealth()) / static_cast<float>(unit.getMaxHealth());
            int frame = std::clamp(static_cast<int>((1.0f - ratio) * 13.0f), 0, 12);
            sf::Sprite healthSprite(healthTex);
            healthSprite.setTextureRect(sf::IntRect({ frame * 32, 0 }, { 32, 32 }));
            healthSprite.setPosition(unit.getPosition());
            target.draw(healthSprite);
        }
    });
}