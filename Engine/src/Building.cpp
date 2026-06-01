#include "Building.hpp"
#include "TextureManager.hpp"
#include "TeamRegistry.hpp"

Building::Building(const std::string& typeName, Team team, const BuildingData& data)
    : typeName(typeName), team(team),
      texturePath(data.texturePath), maskPath(data.maskPath),
    textureRect(data.textureRect), useTextureRect(data.useTextureRect),
      captureThreshold(data.captureThreshold)
{}

const sf::Texture& Building::getTexture() const {
    return TextureManager::getTexture(texturePath, maskPath, TeamRegistry::getColor(team));
}

void Building::onTurnEnd(const Unit* occupant) {
    if (!occupant || occupant->getTeam() == team) {
        captureProgress = 0;
        captureTeam = Team::Neutral;
        return;
    }
    if (!occupant->hasFlag(UnitFlag::Capture)) return;

    if (occupant->getTeam() != captureTeam) {
        captureTeam     = occupant->getTeam();
        captureProgress = 0;
    }

    if (++captureProgress >= captureThreshold) {
        captureProgress = 0;
        setTeam(captureTeam);
        if (onCaptured) onCaptured(captureTeam);
    }
}
