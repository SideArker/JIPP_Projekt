#pragma once

#include "EngineAPI.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>


class ENGINE_API MapRenderer : public sf::Drawable, public sf::Transformable {
private:
	sf::VertexArray m_vertices;
	sf::Texture m_tileset;

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
public:
	MapRenderer();

	bool load(const std::string& tilesetPath, sf::Vector2u tileSize, const std::vector<int>& tiles, unsigned int width, unsigned int height);
};