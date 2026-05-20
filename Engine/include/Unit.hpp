#pragma once

#include "EngineAPI.hpp"
#include <string>
#include <SFML/System/Vector2.hpp>
#include <vector>

enum class MoveDirection {
	Up,
	Down,
	Left,
	Right
};


class ENGINE_API Unit
{
protected:
	std::string name;
	std::string description;
	std::string imagePath;
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
	Unit(const std::string& name, const std::string& imagePath, int health, int damage, int moveSpeed)
		: name(name), imagePath(imagePath), health(health), damage(damage), moveSpeed(moveSpeed) {}

	virtual ~Unit() = default;

	// Methods
	void move(const std::vector<sf::Vector2i>& newPath);
	void update(float deltaTime);

	int takeDamage(int damage);
	int heal(int healAmount);

	// Constant methods
	void dealDamage(Unit& target) const;

	// Getters
	sf::Vector2f getPosition() const { return position; }
	std::string getName() const { return name; }
	int getHealth() const { return health; }
	float getMoveSpeed() const { return moveSpeed; }
	std::string getImagePath() const { return imagePath; }
	MoveDirection getMoveDirection() const { return currentDirection; }

	// Setters
	void setPosition(sf::Vector2f pos) { position = pos; }

};