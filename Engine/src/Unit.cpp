#include "Unit.hpp"
#include <iostream>

static const char* const MoveDirectionNames[] = { "Up", "Down", "Left", "Right" };

MoveDirection castMoveDirection(sf::Vector2f& direction)
{
    if (std::abs(direction.x) > std::abs(direction.y)) {
        return direction.x > 0 ? MoveDirection::Right : MoveDirection::Left;
    }
    else {
        return direction.y > 0 ? MoveDirection::Down : MoveDirection::Up;
    }
}

void Unit::move(const std::vector<sf::Vector2i>& newPath) {

    currentSpeed = 0.0f;
    path = newPath;
}

void Unit::update(float deltaTime) {
    if (path.empty()) {
        currentSpeed = 0.0f;
        return;
    }
    float acceleration = 350.0f;

    sf::Vector2i targetGrid = path.front();
    sf::Vector2f targetPixel(targetGrid.x * tileSize, targetGrid.y * tileSize);

    sf::Vector2f direction = targetPixel - position;

    // Compare only first time for each new path point
    if (directionReset) {
        currentDirection = castMoveDirection(direction);
        std::cout << MoveDirectionNames[static_cast<int>(currentDirection)] << std::endl;
		directionReset = false;
    }

    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

	if (currentSpeed == 0.0f) {
		currentSpeed = startSpeed;
	}

    float maxSpeed = (moveSpeed * tileSize) * 1.05f;
    float deceleration = 350.0f;

    // Total remaining distance: current segment + all further segments
    float remainingDistance = distance;
    for (size_t i = 1; i < path.size(); ++i) {
        sf::Vector2f from(path[i - 1].x * tileSize, path[i - 1].y * tileSize);
        sf::Vector2f to(path[i].x * tileSize, path[i].y * tileSize);
        sf::Vector2f seg = to - from;
        remainingDistance += std::sqrt(seg.x * seg.x + seg.y * seg.y);
    }

    // Braking distance needed to decelerate from current speed to startSpeed
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
		directionReset = true; // Reset direction for the next path point
    }
    else {
        sf::Vector2f normalizedDir = direction / distance;
        position += normalizedDir * moveStep;
    }
}