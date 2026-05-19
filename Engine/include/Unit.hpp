#pragma once

#include "EngineAPI.hpp"
#include <string>
#include <SFML/System/Vector2.hpp>
#include <vector>

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
	float currentSpeed = 0.0f;
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
	std::string getImagePath() const;
	std::string getName() const;
	int getHealth() const;
	void dealDamage(Unit& target) const;

	sf::Vector2f getPosition() const { return position; }
	void setPosition(sf::Vector2f pos) { position = pos; }
	int getMoveSpeed() const { return moveSpeed; }
};