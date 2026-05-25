#pragma once

#include "EngineAPI.hpp"
#include <string>
#include <SFML/Graphics.hpp>
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
	std::string imagePath;
	sf::Texture texture;
	Team team = Team::Neutral;
	std::bitset<static_cast<std::size_t>(UnitFlag::Count)> flags;
	int health;
	int damage;
	int moveSpeed; // In spaces

	std::vector<sf::Vector2i> path;
	sf::Vector2f position = sf::Vector2f(0,0);

	MoveDirection currentDirection = MoveDirection::Right;
	float currentSpeed = 0.0f;
	bool directionReset = true; // Flag to reset direction only on the first update after path change
	float startSpeed = 30.0f;
	float tileSize = 32;
public:
	Unit(const std::string& name, const std::string& artPath, const std::string& maskPath, sf::Color teamColor, int health, int damage, int moveSpeed);

	virtual ~Unit() = default;

	// Methods
	void move(const std::vector<sf::Vector2i>& newPath);
	void update(float deltaTime);

	int takeDamage(int damage);
	int heal(int healAmount);

	// Constant methods
	void dealDamage(Unit& target) const;

	// Flag methods
	void addFlag(UnitFlag flag) { flags.set(static_cast<std::size_t>(flag)); }
	void removeFlag(UnitFlag flag) { flags.reset(static_cast<std::size_t>(flag)); }
	bool hasFlag(UnitFlag flag) const { return flags.test(static_cast<std::size_t>(flag)); }

	// Getters
	sf::Vector2f getPosition() const { return position; }
	std::string getName() const { return name; }
	int getHealth() const { return health; }
	float getMoveSpeed() const { return moveSpeed; }
	std::string getImagePath() const { return imagePath; }
	const sf::Texture& getTexture() const { return texture; }
	MoveDirection getMoveDirection() const { return currentDirection; }
	Team getTeam() const { return team; }

	// Setters
	void setPosition(sf::Vector2f pos) { position = pos; }
	void setTeam(Team t) { team = t; }

};