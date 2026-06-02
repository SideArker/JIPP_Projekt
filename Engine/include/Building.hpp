#pragma once

#include "EngineAPI.hpp"
#include "Unit.hpp"
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

struct ENGINE_API BuildingData {
    std::string texturePath;
    std::string maskPath;
    sf::IntRect textureRect = sf::IntRect({0, 0}, {0, 0});
    bool useTextureRect = false;
    int captureThreshold = 3;
};

class ENGINE_API Building {
protected:
    std::string  typeName;
    Team team;
    sf::Vector2f position;

private:
    std::string texturePath;
    std::string maskPath;
    sf::IntRect textureRect;
    bool useTextureRect;
    int captureThreshold;
    int captureProgress = 0;
    Team captureTeam = Team::Neutral;

public:
    Building(const std::string& typeName, Team team, const BuildingData& data);
    virtual ~Building() = default;

    virtual void onClicked() {}

    // Called at the end of each turn with the unit occupying this tile
    void onTurnEnd(const Unit* occupant);

    std::string  getTypeName()         const { return typeName; }
    Team getTeam() const { return team; }
    sf::Vector2f getPosition() const { return position; }
    int getCaptureProgress() const { return captureProgress; }
    int getCaptureThreshold() const { return captureThreshold; }
    Team getCaptureTeam() const { return captureTeam; }

    void setPosition(sf::Vector2f pos) { position = pos; }
    void setTeam(Team t) { team = t; }

    const sf::Texture& getTexture() const;
    sf::IntRect getTextureRect() const { return textureRect; }
    bool hasTextureRect() const { return useTextureRect; }

    std::function<void(Team newTeam)> onCaptured;
};
