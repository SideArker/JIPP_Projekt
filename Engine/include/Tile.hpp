#pragma once

#include "EngineAPI.hpp"
#include "TerrainMovement.hpp"

class ENGINE_API Tile {
private:
	int artId;
	TerrainType terrain;
public:
	Tile(int artId, TerrainType terrain) : artId(artId), terrain(terrain) {}

	int getArtId() const;
	TerrainType getTerrain() const;
};