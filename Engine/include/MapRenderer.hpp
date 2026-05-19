#pragma once

#include "EngineAPI.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <Tile.hpp>

class MapRenderer : public sf::Drawable, public sf::Transformable {
private:
	sf::VertexArray m_vertices;
	sf::Texture m_tileset;

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
public:
	MapRenderer();

	bool load(const std::string& tilesetPath, sf::Vector2u tileSize, const std::vector<Tile>& tiles, unsigned int width, unsigned int height);
};