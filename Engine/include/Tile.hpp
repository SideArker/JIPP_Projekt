#pragma once

#include "EngineAPI.hpp"
#include "TerrainMovement.hpp"

class ENGINE_API Tile {
private:
	int artId;
	TerrainType terrain;
    unsigned char rotation;
public:
	Tile(int artId = 0, TerrainType terrain = TerrainType::Grass, unsigned char rotation = 0) 
        : artId(artId), terrain(terrain), rotation(rotation) {}

	int getArtId() const;
	TerrainType getTerrain() const;
    unsigned char getRotation() const { return rotation; }
};