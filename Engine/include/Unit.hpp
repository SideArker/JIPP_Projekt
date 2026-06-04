#pragma once

#include "EngineAPI.hpp"
#include "TerrainMovement.hpp"
#include "AnimationManager.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include <bitset>
#include <functional>

enum class MoveDirection {
	Up,
	Down,
	Left,
	Right
};

enum class Team {
	Ally,
	Enemy,
	Neutral
};

struct ENGINE_API UnitData {
	int maxHealth = 1;
	int damage = 1;
	float moveSpeed = 3.f;
	int minAttackRange = 0;
	int maxAttackRange = 1;
	int cost = 100;
	float hitEffectDelay = 0.0f;
	float attackDamageDelay = 0.75f;
	MovementCategory movementCategory = MovementCategory::Ground;
	std::string deathEffectSet;
	std::string deathEffectClip;
	std::string deathEffectTexturePath;
	std::string deathSoundSet;
	std::string deathSoundName;
};

enum class UnitFlag {
	DamageMultToInfantry,
	Capture,
	IsArmored,

	CanAttackGround,
	CanAttackInfantry,
	CanAttackFlying,
	CanAttackNaval,

	// Always keep this last!
	Count
};


class ENGINE_API Unit
{
protected:
	std::string name;
	std::string description;
	std::string artPath;
	std::string maskPath;
	const sf::Texture* texture;
	const AnimationSet* animSet;
	AnimationState animState;
	Team team = Team::Neutral;
	std::bitset<static_cast<std::size_t>(UnitFlag::Count)> flags;
	int health;
	int maxHealth;
	int damage;
	float moveSpeed;
	int minAttackRange;
	int maxAttackRange;
	float hitEffectDelay;
	MovementCategory movementCategory = MovementCategory::Ground;

	std::vector<sf::Vector2i> path;
	sf::Vector2f position = sf::Vector2f(0, 0);

	MoveDirection currentDirection = MoveDirection::Right;
	float currentSpeed = 0.0f;
	bool directionReset = true;
	float startSpeed = 30.0f;
	float tileSize = 32;
	bool m_isShooting = false;
	bool m_shootPending = false;
	MoveDirection m_pendingShootDir = MoveDirection::Right;
	bool m_isDead = false;
	bool m_hasActed = false;
	std::weak_ptr<Unit> m_pendingTarget;
public:
	Unit(const std::string& name, const std::string& artPath, const std::string& maskPath, const AnimationSet& animSet, Team team, const UnitData& data);

	virtual ~Unit() = default;

	void move(const std::vector<sf::Vector2i>& newPath);
	void update(float deltaTime);
    void setDirection(MoveDirection dir);
    MoveDirection getDirection() const { return currentDirection; }

	int takeDamage(int damage);
	int heal(int healAmount);

	void dealDamage(std::shared_ptr<Unit> target, MoveDirection shootDir);
	bool isActing() const { return !m_isDead && (!path.empty() || m_isShooting || m_shootPending); }
	bool canTarget(const Unit& target) const;

	void addFlag(UnitFlag flag) { flags.set(static_cast<std::size_t>(flag)); }
	void removeFlag(UnitFlag flag) { flags.reset(static_cast<std::size_t>(flag)); }
	bool hasFlag(UnitFlag flag) const { return flags.test(static_cast<std::size_t>(flag)); }

	sf::Vector2f getPosition() const { return position; }
	std::string getName() const { return name; }
	std::string getArtPath() const { return artPath; }
	int getHealth() const { return health; }
	int getMaxHealth() const { return maxHealth; }
	int getDamage() const { return damage; }

	float getMoveSpeed() const { return moveSpeed; }
	MovementCategory getMovementCategory() const { return movementCategory; }
	int   getMinAttackRange()   const { return minAttackRange; }
	int   getMaxAttackRange()   const { return maxAttackRange; }
	float getHitEffectDelay()   const { return hitEffectDelay; }
	const sf::Texture& getTexture() const { return *texture; }
	const sf::Texture& getCurrentTexture() const;
	sf::IntRect getCurrentRect() const { return animState.getCurrentRect(); }
	bool shouldFlipX() const { return animState.shouldFlipX(); }
	MoveDirection getMoveDirection() const { return currentDirection; }
	Team getTeam() const { return team; }
	uint8_t getFlags() const { return static_cast<uint8_t>(flags.to_ulong()); }
	sf::Vector2i getGridPosition(sf::Vector2u tileSize) const;

	void setPosition(sf::Vector2f pos) { position = pos; }
	void setTeam(Team t);
	void setHealth(int h) { health = std::clamp(h, 0, maxHealth); }
	void setDamage(int d) { damage = d; }
	void setMoveSpeed(float speed) { moveSpeed = speed; }

	void setActed(bool acted) { m_hasActed = acted; }

	bool isDead() const { return m_isDead; }
	bool hasActed() const { return m_hasActed; }
	void setFlags(uint8_t f) { flags = std::bitset<static_cast<std::size_t>(UnitFlag::Count)>(f); }

	std::function<void(sf::Vector2f, int health)> onDamaged;	
	std::function<void(std::shared_ptr<Unit>, int)> onAttackStart;
	std::function<void()> onAttackFinished;
};