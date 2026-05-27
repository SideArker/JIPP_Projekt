#pragma once

#include "EngineAPI.hpp"
#include "Unit.hpp"
#include <SFML/Graphics.hpp>
#include <map>

class ENGINE_API TeamRegistry {
public:
    static void setColor(Team team, sf::Color color);

    // Returns sf::Color::White if no color is registered for the team.
    static sf::Color getColor(Team team);
};
