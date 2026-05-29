#include "GameContent.hpp"
#include "TeamRegistry.hpp"
#include "AnimationManager.hpp"
#include "SoundManager.hpp"
#include "units/Tank.hpp"
#include "units/Soldier.hpp"
#include "effects/Effects.hpp"
#include <SFML/Graphics.hpp>

void GameContent::init() {
    TeamRegistry::setColor(Team::Ally,    sf::Color( 50, 255,  50));
    TeamRegistry::setColor(Team::Enemy,   sf::Color(255,   7,  58));
    TeamRegistry::setColor(Team::Neutral, sf::Color(200, 200, 200));

    registerTank();
    registerSoldier();

    registerEffects();

    SoundManager::registerMusic("MainMenu",  "Art/Sound/main_menu.ogg");
    SoundManager::registerMusic("EnemyTurn", "Art/Sound/enemy_turn.ogg");
    SoundManager::registerMusic("AllyTurn",  "Art/Sound/ally_turn.ogg");
}