#pragma once

#include "AnimationManager.hpp"
#include <memory>
#include <functional>

class Unit;
enum class MoveDirection;

class UnitAnimator {
public:
    static std::string dirSuffix(MoveDirection dir);
    static std::string clipName(const std::string& action, MoveDirection dir);

    static void handleShootAnimation(
        AnimationState& animState, const AnimationSet* animSet, 
        bool& m_shootPending, bool& m_isShooting, MoveDirection& currentDirection, MoveDirection m_pendingShootDir, 
        std::weak_ptr<Unit>& m_pendingTarget, int damage,
        const std::function<void(std::shared_ptr<Unit>, int)>& onAttackStart,
        std::function<void()>& onAttackFinished, float deltaTime);

    static void handleIdleWalkAnimation(
        AnimationState& animState, const AnimationSet* animSet, MoveDirection currentDirection, float deltaTime);
};
