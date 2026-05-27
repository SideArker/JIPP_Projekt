#include "Unit.hpp"
#include <cmath>

static std::string clipNameForDirection(MoveDirection dir) {
    switch (dir) {
        case MoveDirection::Left:  return "move_left";
        case MoveDirection::Right: return "move_right";
        case MoveDirection::Down:  return "move_down";
        case MoveDirection::Up:    return "move_up";
    }
    return "idle";
}

Unit::Unit(const std::string& name, const sf::Texture& texture, const AnimationSet& animSet, int health, int damage, int moveSpeed)
    : name(name), texture(&texture), animSet(&animSet), health(health), damage(damage), moveSpeed(moveSpeed)
{
    animState.play(clipNameForDirection(currentDirection), *this->animSet);
}

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
        animState.play("idle", *animSet);
        animState.update(deltaTime);
        return;
    }
    float acceleration = 350.0f;

    sf::Vector2i targetGrid = path.front();
    sf::Vector2f targetPixel(targetGrid.x * tileSize, targetGrid.y * tileSize);

    sf::Vector2f direction = targetPixel - position;

    if (directionReset) {
        currentDirection = castMoveDirection(direction);
        directionReset = false;
        animState.play(clipNameForDirection(currentDirection), *animSet);
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
		directionReset = true;
    }
    else {
        sf::Vector2f normalizedDir = direction / distance;
        position += normalizedDir * moveStep;
    }

    animState.update(deltaTime);
}

sf::Vector2i Unit::getGridPosition(sf::Vector2u ts) const {
    return {
        static_cast<int>(std::round(position.x / static_cast<float>(ts.x))),
        static_cast<int>(std::round(position.y / static_cast<float>(ts.y)))
    };
}

int Unit::takeDamage(int damage) {
    health = std::max(0, health - damage);
    return health;
}

int Unit::heal(int amount) {
    health += amount;
    return health;
}

void Unit::dealDamage(Unit& target) const {
    target.takeDamage(damage);
}