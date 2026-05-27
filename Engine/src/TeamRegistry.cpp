#include "TeamRegistry.hpp"
#include <map>

static std::map<Team, sf::Color> s_colors;

void TeamRegistry::setColor(Team team, sf::Color color) {
    s_colors[team] = color;
}

sf::Color TeamRegistry::getColor(Team team) {
    auto it = s_colors.find(team);
    return it != s_colors.end() ? it->second : sf::Color::White;
}
