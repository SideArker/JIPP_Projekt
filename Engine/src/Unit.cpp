#include "Unit.hpp"

void Unit::move(const std::vector<sf::Vector2i>& newPath) {

    path = newPath;
}

std::string Unit::getImagePath() const {
	return imagePath;
}

void Unit::update(float deltaTime) {
    if (path.empty()) {
        return;
    }

    sf::Vector2i targetGrid = path.front();
    sf::Vector2f targetPixel(targetGrid.x * tileSize, targetGrid.y * tileSize);

    sf::Vector2f direction = targetPixel - position;
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    float speedInPixels = (moveSpeed * tileSize) * 1.5f;
    float moveStep = speedInPixels * deltaTime;

    if (distance <= moveStep) {
        position = targetPixel;
        path.erase(path.begin());
    }
    else {
        sf::Vector2f normalizedDir = direction / distance;
        position += normalizedDir * moveStep;
    }
}