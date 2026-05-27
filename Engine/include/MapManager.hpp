#pragma once

#include "MapRenderer.hpp"
#include "EngineAPI.hpp"
#include "GameState.hpp"
#include <Unit.hpp>
#include <memory>
#include <string>
#include <vector>

class SelectionController;

class ENGINE_API MapManager {
private:
	MapRenderer renderer;
	std::vector<std::shared_ptr<Unit>> units;
	std::vector<Tile> mapData;
	std::unique_ptr<SelectionController> selectionController;
	sf::Vector2u tileSize;
	unsigned int mapWidth;
	unsigned int mapHeight;
	std::string  tilesetPath;
	std::string  currentMapPath;

public:
	MapManager();
	~MapManager();

	bool loadFromFile(const std::string& mapPath);
	bool saveToFile(const std::string& mapPath) const;
	GameState captureGameState() const;
	bool restoreGameState(const std::string& savePath);

	bool loadMap(const std::string& tileset, sf::Vector2u tileSize, const std::vector<Tile>& tiles, unsigned int w, unsigned int h);
	void spawnUnit(std::shared_ptr<Unit> unit, int gridX, int gridY);
	void setupInput(sf::RenderWindow& window);
	void handleEvent(const sf::Event& event);
	void update(float deltaTime);
	void draw(sf::RenderTarget& target);
	void drawUI();
	std::vector<sf::Vector2i> findPath(sf::Vector2i start, sf::Vector2i goal, Team movingTeam);
	std::vector<sf::Vector2i> getReachableTiles(sf::Vector2i from, int moveRange, Team movingTeam) const;
	std::shared_ptr<Unit> getUnitAtTile(sf::Vector2i gridPos) const;
	sf::Vector2u getTileSize() const;
	unsigned int getMapWidth() const;
	unsigned int getMapHeight() const;
};