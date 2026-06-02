	#include "MapRenderer.hpp"
	#include <iostream>

	MapRenderer::MapRenderer() : m_vertices(sf::PrimitiveType::Triangles) {}

	bool MapRenderer::load(const std::string& tilesetPath, sf::Vector2u tileSize, const std::vector<Tile>& tiles, unsigned int width, unsigned int height) {
		if (!m_tileset.loadFromFile(tilesetPath)) {
			return false;
		}
		m_tileset.setSmooth(false);
		m_vertices.resize(width * height * 6); // 6 vertices per tile (2 triangles)


		for (unsigned int i = 0; i < width; ++i) {
			for (unsigned int j = 0; j < height; ++j) {
				int tileNumber = tiles[i + (j * width)].getArtId();
				int rotation = 0; // 0 = 0 deg, 1 = 90 deg CW, 2 = 180 deg, 3 = 270 deg CW

				if (tileNumber == 0) continue; // Skip empty tiles

				int tu = (tileNumber - 1) % (m_tileset.getSize().x / tileSize.x);
				int tv = (tileNumber - 1) / (m_tileset.getSize().x / tileSize.x);

				sf::Vertex* triangles = &m_vertices[(i + j * width) * 6];

				// Triangle 1
				triangles[0].position = sf::Vector2f(i * tileSize.x, j * tileSize.y);             // Top-Left
				triangles[1].position = sf::Vector2f((i + 1) * tileSize.x, j * tileSize.y);       // Top-Right
				triangles[2].position = sf::Vector2f(i * tileSize.x, (j + 1) * tileSize.y);       // Bottom-Left

				// Triangle 2
				triangles[3].position = sf::Vector2f(i * tileSize.x, (j + 1) * tileSize.y);       // Bottom-Left
				triangles[4].position = sf::Vector2f((i + 1) * tileSize.x, j * tileSize.y);       // Top-Right
				triangles[5].position = sf::Vector2f((i + 1) * tileSize.x, (j + 1) * tileSize.y); // Bottom-Right

				float tu0 = tu * tileSize.x;
				float tv0 = tv * tileSize.y;
				float tu1 = (tu + 1) * tileSize.x;
				float tv1 = (tv + 1) * tileSize.y;

				// Texture Coords: Triangle 1
				triangles[0].texCoords = sf::Vector2f(tu0, tv0); // Top-Left
				triangles[1].texCoords = sf::Vector2f(tu1, tv0); // Top-Right
				triangles[2].texCoords = sf::Vector2f(tu0, tv1); // Bottom-Left

				// Texture Coords: Triangle 2
				triangles[3].texCoords = sf::Vector2f(tu0, tv1); // Bottom-Left
				triangles[4].texCoords = sf::Vector2f(tu1, tv0); // Top-Right
				triangles[5].texCoords = sf::Vector2f(tu1, tv1); // Bottom-Right

			}
		}
		return true;
	}

	void MapRenderer::draw(sf::RenderTarget& target, sf::RenderStates states) const {
		states.transform *= getTransform();
		states.texture = &m_tileset;
		target.draw(m_vertices, states);
	}