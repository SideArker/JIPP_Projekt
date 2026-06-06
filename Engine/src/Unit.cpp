#include "Unit.hpp"
#include "UnitAnimator.hpp"
#include "TextureManager.hpp"
#include "TeamRegistry.hpp"
#include "SoundManager.hpp"
#include <iostream>
#include <cmath>

Unit::Unit(const std::string& name, const std::string& artPath, const std::string& maskPath,
           const AnimationSet& animSet, Team team,
           const UnitData& data)
    : name(name), artPath(artPath), maskPath(maskPath),
      animSet(&animSet), team(team),
      health(data.maxHealth), maxHealth(data.maxHealth), damage(data.damage),
      moveSpeed(data.moveSpeed), minAttackRange(data.minAttackRange), maxAttackRange(data.maxAttackRange),
      hitEffectDelay(data.hitEffectDelay), movementCategory(data.movementCategory),
      isInteractable(data.isInteractable)
{
    texture = &TextureManager::getTexture(artPath, maskPath, TeamRegistry::getColor(team));
    const std::string directionalIdle = UnitAnimator::clipName("idle", currentDirection);
    const std::string directionalWalk = UnitAnimator::clipName("walk", currentDirection);
    if (this->animSet->getClip(directionalIdle)) {
        animState.play(directionalIdle, *this->animSet);
    } else if (this->animSet->getClip("idle")) {
        animState.play("idle", *this->animSet);
    } else if (this->animSet->getClip(directionalWalk)) {
        animState.play(directionalWalk, *this->animSet);
        animState.resetToFrameZero();
    } else if (this->animSet->getClip("walk")) {
        animState.play("walk", *this->animSet);
        animState.resetToFrameZero();
    }
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

void Unit::setDirection(MoveDirection dir) {
    currentDirection = dir;
    const std::string directionalIdle = UnitAnimator::clipName("idle", currentDirection);
    const std::string directionalWalk = UnitAnimator::clipName("walk", currentDirection);
    if (this->animSet->getClip(directionalIdle)) {
        animState.play(directionalIdle, *this->animSet);
    } else if (this->animSet->getClip("idle")) {
        animState.play("idle", *this->animSet);
    } else if (this->animSet->getClip(directionalWalk)) {
        animState.play(directionalWalk, *this->animSet);
        animState.resetToFrameZero();
    } else if (this->animSet->getClip("walk")) {
        animState.play("walk", *this->animSet);
        animState.resetToFrameZero();
    }
}

void Unit::move(const std::vector<sf::Vector2i>& newPath) {
    m_movement.startMove(newPath);
    SoundManager::play(name, "move");
}

void Unit::update(float deltaTime) {
    if (spawnFadeTimer > 0.f) {
        spawnFadeTimer = std::max(0.f, spawnFadeTimer - deltaTime);
    }

    if (!m_movement.isMoving()) {
        int scaledDamage = static_cast<int>(std::ceil(static_cast<float>(damage) * (static_cast<float>(health) / maxHealth)));
        UnitAnimator::handleShootAnimation(
            animState, animSet, 
            m_shootPending, m_isShooting, currentDirection, m_pendingShootDir, 
            m_pendingTarget, scaledDamage, onAttackStart, onAttackFinished, deltaTime);
            
        if (!m_isShooting) {
            UnitAnimator::handleIdleWalkAnimation(animState, animSet, currentDirection, deltaTime);
        }
        return;
    }
    
    if (m_movement.update(position, currentDirection, moveSpeed, tileSize, deltaTime)) {
        // Animation update is done via play if direction changed, but we should always update dt
        animState.play(UnitAnimator::clipName("walk", currentDirection), *animSet);
        animState.update(deltaTime);
    }
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

bool Unit::canTarget(const Unit& target) const {
    switch (target.getMovementCategory()) {
        case MovementCategory::None:
        case MovementCategory::Ground:   return hasFlag(UnitFlag::CanAttackGround);
        case MovementCategory::Infantry: return hasFlag(UnitFlag::CanAttackInfantry);
        case MovementCategory::Flying:   return hasFlag(UnitFlag::CanAttackFlying);
        case MovementCategory::Naval:    return hasFlag(UnitFlag::CanAttackNaval);
    }
    return false;
}