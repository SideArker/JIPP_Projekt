#pragma once

#include "MapRenderer.hpp"
#include "AnimationManager.hpp"
#include "EngineAPI.hpp"
#include "GameState.hpp"
#include "TurnController.hpp"
#include <Unit.hpp>
#include "Building.hpp"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class SelectionController;

struct Effect {
	sf::Sprite sprite;
	AnimationState animState;
	const AnimationSet* animSet = nullptr;
	std::function<void()> onFinished;
};

class ENGINE_API MapManager {
private:
	MapRenderer renderer;
	std::unique_ptr<SelectionController> selectionController;

	std::vector<std::shared_ptr<Unit>> units;
	std::vector<std::shared_ptr<Building>> buildings;
	std::vector<Tile> mapData;
	std::string  tilesetPath;
	sf::Vector2u tileSize;
	unsigned int mapWidth;
	unsigned int mapHeight;
	std::string  currentMapPath;

	std::string m_hitEffectSet;
	std::string m_hitEffectClip;
	std::string m_hitEffectTexturePath;
	std::string m_teamCaptureTexturePath;
	std::string m_teamCaptureMaskPath;
	std::map<std::string, sf::Texture> m_effectTextures;
	std::function<void(sf::RenderTarget&, const Unit&, bool)> m_unitRenderCallback;
	std::string m_walkOverlayPath;
	std::string m_moveArrowPath;
	std::string m_iconsPath;
	std::string m_enemyOverlayPath;
	float m_captureBounceTimer = 0.f;
	std::unordered_set<const Building*> m_captureBounceTargets;

	std::vector<Effect> m_effects;
	std::vector<std::pair<float, std::function<void()>>> m_pendingActions;
	std::vector<std::function<void()>> m_whenIdleActions;
	TurnController m_turnController;

	void spawnEffect(const std::string& setName, const std::string& clipName, const std::string& texturePath, sf::Vector2f position, float yOffset = 0.f);
	void spawnHitEffect(sf::Vector2f position);
	void spawnDeathEffect(const std::string& setName, const std::string& clipName, const std::string& texturePath, const std::string& soundSet, const std::string& soundName, sf::Vector2f position, std::shared_ptr<Unit> unit);

public:
	MapManager();
	~MapManager();

	bool loadFromFile(const std::string& mapPath);
	bool saveToFile(const std::string& mapPath) const;
	GameState captureGameState() const;
	bool restoreGameState(const std::string& savePath);

	bool loadMap(const std::string& tileset, sf::Vector2u tileSize, const std::vector<Tile>& tiles, unsigned int w, unsigned int h);
	void spawnUnit(std::shared_ptr<Unit> unit, int gridX, int gridY);
	void spawnBuilding(std::shared_ptr<Building> building, int gridX, int gridY);
	std::shared_ptr<Building> getBuildingAtTile(sf::Vector2i gridPos) const;
	const std::vector<std::shared_ptr<Building>>& getBuildings() const { return buildings; }
	void setupInput(sf::RenderWindow& window);
	void handleEvent(const sf::Event& event);
	void update(float deltaTime);
	void draw(sf::RenderTarget& target);
	void drawUI();
	bool isAnyUnitActing() const;
	void runWhenAllActionsFinished(std::function<void()> action);
	void endTurn();
	void requestEndTurn();
	Team getCurrentTeam() const;
	TurnController& getTurnController();
	tgui::Gui* getGui();
	void syncCameraView(const sf::View& gameView);

	void setHitEffect(std::string setName, std::string clipName, std::string texturePath);
	void setTeamCaptureEffect(std::string texturePath, std::string maskPath);
	void setUnitRenderCallback(std::function<void(sf::RenderTarget&, const Unit&, bool)> cb);
	void setWalkOverlayPath(std::string path);
	void setMoveArrowPath(std::string path);
	void setIconsPath(std::string path);
	void setEnemyOverlayPath(std::string path);

	const std::vector<std::shared_ptr<Unit>>& getUnits() const { return units; }

	std::vector<sf::Vector2i> findPath(sf::Vector2i start, sf::Vector2i goal, Team movingTeam, MovementCategory category);
	std::vector<sf::Vector2i> getReachableTiles(sf::Vector2i from, int moveRange, Team movingTeam, MovementCategory category) const;
	std::shared_ptr<Unit> getUnitAtTile(sf::Vector2i gridPos) const;
	sf::Vector2u getTileSize() const;
	unsigned int getMapWidth() const;
	unsigned int getMapHeight() const;
};