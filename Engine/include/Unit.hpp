#pragma once

#include "EngineAPI.hpp"
#include <string>

class ENGINE_API Unit
{
protected:
	std::string name;
	int health;
	int damage;
	int moveSpeed; // In spaces

public:
	Unit(const std::string& name, int health, int damage, int moveSpeed)
		: name(name), health(health), damage(damage), moveSpeed(moveSpeed) {
	}

	virtual ~Unit() = default;

	// Methods
	void move();
	int takeDamage(int damage);
	int heal(int healAmount);

	// Constant methods
	std::string getName() const;
	int getHealth() const;
	void dealDamage(Unit& target) const;
};