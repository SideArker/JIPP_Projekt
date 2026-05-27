#pragma once

#include "EngineAPI.hpp"
#include "AnimationManager.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <bitset>

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

enum class UnitFlag {
	DamageMultToInfantry,
	Capture,
	CanFly,
	IsArmored,

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
	int damage;
	int moveSpeed;

	std::vector<sf::Vector2i> path;
	sf::Vector2f position = sf::Vector2f(0, 0);

	MoveDirection currentDirection = MoveDirection::Right;
	float currentSpeed = 0.0f;
	bool directionReset = true;
	float startSpeed = 30.0f;
	float tileSize = 32;
public:
	Unit(const std::string& name, const std::string& artPath, const std::string& maskPath, const AnimationSet& animSet, Team team, int health, int damage, int moveSpeed);

	virtual ~Unit() = default;

	void move(const std::vector<sf::Vector2i>& newPath);
	void update(float deltaTime);

	int takeDamage(int damage);
	int heal(int healAmount);

	void dealDamage(Unit& target) const;

	void addFlag(UnitFlag flag) { flags.set(static_cast<std::size_t>(flag)); }
	void removeFlag(UnitFlag flag) { flags.reset(static_cast<std::size_t>(flag)); }
	bool hasFlag(UnitFlag flag) const { return flags.test(static_cast<std::size_t>(flag)); }

	sf::Vector2f getPosition() const { return position; }
	std::string getName() const { return name; }
	int getHealth() const { return health; }
	float getMoveSpeed() const { return moveSpeed; }
	const sf::Texture& getTexture() const { return *texture; }
	sf::IntRect getCurrentRect() const { return animState.getCurrentRect(); }
	bool shouldFlipX() const { return animState.shouldFlipX(); }
	MoveDirection getMoveDirection() const { return currentDirection; }
	Team getTeam() const { return team; }
	int getDamage() const { return damage; }
	uint8_t getFlags() const { return static_cast<uint8_t>(flags.to_ulong()); }
	sf::Vector2i getGridPosition(sf::Vector2u tileSize) const;

	void setPosition(sf::Vector2f pos) { position = pos; }
	void setTeam(Team t);
	void setHealth(int h) { health = h; }
	void setDamage(int d) { damage = d; }
	void setMoveSpeed(int s) { moveSpeed = s; }
	void setFlags(uint8_t f) { flags = std::bitset<static_cast<std::size_t>(UnitFlag::Count)>(f); }
};