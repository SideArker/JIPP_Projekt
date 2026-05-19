#include "EngineAPI.hpp"
class ENGINE_API Tile {
private:
	int artId;
	bool walkable;
public:
	Tile(int artId, bool walkable) : artId(artId), walkable(walkable) {}

	int getArtId() const;
	bool isWalkable() const;
};