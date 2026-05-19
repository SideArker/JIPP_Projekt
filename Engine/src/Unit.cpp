#include "Unit.hpp"
#include <iostream>
void Unit::move(const std::vector<sf::Vector2i>& newPath) {

    currentSpeed = 0.0f;
    path = newPath;
}

std::string Unit::getImagePath() const {
	return imagePath;
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
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    float maxSpeed = (moveSpeed * tileSize) * 1.05f;
    std::cout << currentSpeed << std::endl;

	currentSpeed += acceleration * deltaTime;
	if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;

    float moveStep = currentSpeed * deltaTime;

    if (distance <= moveStep) {
        position = targetPixel;
        path.erase(path.begin());
    }
    else {
        sf::Vector2f normalizedDir = direction / distance;
        position += normalizedDir * moveStep;
    }
}