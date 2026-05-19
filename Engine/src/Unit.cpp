#include "Unit.hpp"
#include <iostream>

static const char* const MoveDirectionNames[] = { "Up", "Down", "Left", "Right" };

MoveDirection getMoveDirection(sf::Vector2f& direction)
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
        currentDirection = getMoveDirection(direction);
        std::cout << MoveDirectionNames[static_cast<int>(currentDirection)] << std::endl;
		directionReset = false;
    }

    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

	if (currentSpeed == 0.0f) {
		currentSpeed = startSpeed;
	}

    float maxSpeed = (moveSpeed * tileSize) * 1.05f;

	currentSpeed += acceleration * deltaTime;
	if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;

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