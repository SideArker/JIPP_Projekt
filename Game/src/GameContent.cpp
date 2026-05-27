#include "GameContent.hpp"
#include "TeamRegistry.hpp"
#include "units/Tank.hpp"
#include <SFML/Graphics.hpp>

void GameContent::init() {
    TeamRegistry::setColor(Team::Ally,    sf::Color( 50, 255,  50));
    TeamRegistry::setColor(Team::Enemy,   sf::Color(255,   7,  58));
    TeamRegistry::setColor(Team::Neutral, sf::Color(200, 200, 200));

    registerTank();
}