#include "Unit.hpp"
#include "TextureManager.hpp"
#include "TeamRegistry.hpp"
#include <iostream>
#include <cmath>

static const char* dirSuffix(MoveDirection dir) {
    switch (dir) {
        case MoveDirection::Left:  return "_left";
        case MoveDirection::Right: return "_right";
        case MoveDirection::Down:  return "_down";
        case MoveDirection::Up:    return "_up";
    }
    return "";
}

static std::string clipName(const std::string& action, MoveDirection dir) {
    return action + dirSuffix(dir);
}

Unit::Unit(const std::string& name, const std::string& artPath, const std::string& maskPath,
           const AnimationSet& animSet, Team team,
           int health, int damage, int moveSpeed)
    : name(name), artPath(artPath), maskPath(maskPath),
      animSet(&animSet), team(team),
    health(health), maxHealth(health), damage(damage), moveSpeed(moveSpeed)
{
    texture = &TextureManager::getTexture(artPath, maskPath, TeamRegistry::getColor(team));
    if(team == Team::Ally) animState.play(clipName("idle", currentDirection), *this->animSet);
}

void Unit::setTeam(Team t) {
    team = t;
    texture = &TextureManager::getTexture(artPath, maskPath, TeamRegistry::getColor(t));
}

const sf::Texture& Unit::getCurrentTexture() const {
    const AnimationClip* clip = animState.getCurrentClip();
    if (clip && !clip->texturePath.empty())
        return TextureManager::getTexture(clip->texturePath, clip->maskPath, TeamRegistry::getColor(team));
    return *texture;
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
        // Activate shoot animation once the unit has finished moving
        if (m_shootPending) {
            m_shootPending = false;
            m_isShooting = true;
            auto target = m_pendingTarget.lock();
            if (target && !target->isDead()) {
                if (onAttackStart)
                    onAttackStart(target, damage);
                else
                    target->takeDamage(damage);
            }
            m_pendingTarget.reset();
            currentDirection = m_pendingShootDir;
            const std::string clip = clipName("shoot", m_pendingShootDir);
            if (!animSet->getClip(clip))
                m_isShooting = false;
            else
                animState.play(clip, *animSet, [this]() { m_isShooting = false; });
        }
        if (m_isShooting) {
            animState.update(deltaTime);
            return;
        }
        currentSpeed = 0.0f;

        if (team == Team::Ally) {
            animState.play("idle", *animSet);
        }
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
        animState.play(clipName("idle", currentDirection), *animSet);
    }

    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

	if (currentSpeed == 0.0f) {
		currentSpeed = startSpeed;
	}

    float maxSpeed = (moveSpeed * tileSize) * 1.05f;
    float deceleration = 350.0f;

    // Total remaining distance current segment + all further segments
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
    onDamaged(position, health);

    return health;
}

int Unit::heal(int amount) {
    health += amount;
    return health;
}

void Unit::dealDamage(std::shared_ptr<Unit> target, MoveDirection shootDir) {
    m_pendingShootDir = shootDir;
    m_pendingTarget = target;
    m_shootPending = true;
}