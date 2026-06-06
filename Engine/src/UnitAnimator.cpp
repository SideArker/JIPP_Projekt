#include "UnitAnimator.hpp"
#include "Unit.hpp"

std::string UnitAnimator::dirSuffix(MoveDirection dir) {
    switch (dir) {
        case MoveDirection::Left:  return "_left";
        case MoveDirection::Right: return "_right";
        case MoveDirection::Down:  return "_down";
        case MoveDirection::Up:    return "_up";
    }
    return "";
}

std::string UnitAnimator::clipName(const std::string& action, MoveDirection dir) {
    return action + dirSuffix(dir);
}

void UnitAnimator::handleShootAnimation(
    AnimationState& animState, const AnimationSet* animSet, 
    bool& m_shootPending, bool& m_isShooting, MoveDirection& currentDirection, MoveDirection m_pendingShootDir, 
    std::weak_ptr<Unit>& m_pendingTarget, int damage,
    const std::function<void(std::shared_ptr<Unit>, int)>& onAttackStart,
    std::function<void()>& onAttackFinished, float deltaTime)
{
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

        if (!animSet->getClip(clip)) {
            m_isShooting = false;
            if (onAttackFinished) {
                onAttackFinished();
                onAttackFinished = nullptr; // Clean up
            }
        } else {
            animState.play(clip, *animSet, [&m_isShooting, &onAttackFinished]() { 
                m_isShooting = false; 
                if (onAttackFinished) {
                    onAttackFinished();
                    onAttackFinished = nullptr; // Clean up
                }
            });
        }
    }
    if (m_isShooting) {
        animState.update(deltaTime);
    }
}

void UnitAnimator::handleIdleWalkAnimation(
    AnimationState& animState, const AnimationSet* animSet, MoveDirection currentDirection, float deltaTime)
{
    bool isFallbackWalk = false;
    const std::string directionalIdle = clipName("idle", currentDirection);
    const std::string directionalWalk = clipName("walk", currentDirection);
    const AnimationClip* targetIdleClip = animSet->getClip(directionalIdle);
    
    if (targetIdleClip) {
        if (animState.getCurrentClip() != targetIdleClip) {
            animState.play(directionalIdle, *animSet);
        }
    } else if ((targetIdleClip = animSet->getClip("idle")) != nullptr) {
        if (animState.getCurrentClip() != targetIdleClip) {
            animState.play("idle", *animSet);
        }
    } else if ((targetIdleClip = animSet->getClip(directionalWalk)) != nullptr) {
        if (animState.getCurrentClip() != targetIdleClip) {
            animState.play(directionalWalk, *animSet);
        }
        isFallbackWalk = true;
    } else if ((targetIdleClip = animSet->getClip("walk")) != nullptr) {
        if (animState.getCurrentClip() != targetIdleClip) {
            animState.play("walk", *animSet);
        }
        isFallbackWalk = true;
    }

    if (isFallbackWalk) {
        animState.resetToFrameZero();
    } else {
        animState.update(deltaTime);
    }
}
