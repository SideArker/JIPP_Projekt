#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

enum class MoveDirection;

struct UnitMovement {
    std::vector<sf::Vector2i> path;
    float currentSpeed = 0.0f;
    bool directionReset = true;
    float startSpeed = 30.0f;

    void startMove(const std::vector<sf::Vector2i>& newPath) {
        currentSpeed = 0.0f;
        path = newPath;
        directionReset = true;
    }

    bool update(sf::Vector2f& position, MoveDirection& currentDirection, float moveSpeed, float tileSize, float deltaTime);
    bool isMoving() const { return !path.empty(); }
};
