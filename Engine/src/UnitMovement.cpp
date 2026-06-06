#include "UnitMovement.hpp"
#include "Unit.hpp" // For MoveDirection definition
#include <cmath>
#include <algorithm>

MoveDirection castMoveDir(sf::Vector2f& direction) {
    if (std::abs(direction.x) > std::abs(direction.y)) {
        return direction.x > 0 ? MoveDirection::Right : MoveDirection::Left;
    } else {
        return direction.y > 0 ? MoveDirection::Down : MoveDirection::Up;
    }
}

bool UnitMovement::update(sf::Vector2f& position, MoveDirection& currentDirection, float moveSpeed, float tileSize, float deltaTime) {
    if (path.empty()) return false;

    float acceleration = 350.0f;
    sf::Vector2i targetGrid = path.front();
    sf::Vector2f targetPixel(targetGrid.x * tileSize, targetGrid.y * tileSize);

    sf::Vector2f direction = targetPixel - position;

    if (directionReset) {
        currentDirection = castMoveDir(direction);
        directionReset = false;
        // Animation trigger is handled externally now
    }

    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    if (currentSpeed == 0.0f) {
        currentSpeed = startSpeed;
    }

    float maxSpeed = (moveSpeed * tileSize) * 1.05f;
    float deceleration = 350.0f;

    float remainingDistance = distance;
    for (size_t i = 1; i < path.size(); ++i) {
        sf::Vector2f from(path[i - 1].x * tileSize, path[i - 1].y * tileSize);
        sf::Vector2f to(path[i].x * tileSize, path[i].y * tileSize);
        sf::Vector2f seg = to - from;
        remainingDistance += std::sqrt(seg.x * seg.x + seg.y * seg.y);
    }

    float brakingDistance = (currentSpeed * currentSpeed - startSpeed * startSpeed) / (2.0f * deceleration);

    if (remainingDistance <= brakingDistance) {
        currentSpeed -= deceleration * deltaTime;
        if (currentSpeed < startSpeed) currentSpeed = startSpeed;
    } else {
        currentSpeed += acceleration * deltaTime;
        if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;
    }

    float moveStep = currentSpeed * deltaTime;

    if (distance <= moveStep) {
        position = targetPixel;
        path.erase(path.begin());
        directionReset = true;
    } else {
        sf::Vector2f normalizedDir = direction / distance;
        position += normalizedDir * moveStep;
    }

    return true; // Still moving
}
