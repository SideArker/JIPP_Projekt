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
				uint8_t rotation = tiles[i + (j * width)].getRotation();

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

                sf::Vector2f tl(tu0, tv0);
                sf::Vector2f tr(tu1, tv0);
                sf::Vector2f bl(tu0, tv1);
                sf::Vector2f br(tu1, tv1);

                sf::Vector2f t_tl, t_tr, t_bl, t_br;
                if (rotation == 0) {
                    t_tl = tl; t_tr = tr; t_bl = bl; t_br = br;
                } else if (rotation == 1) { // 90 CW
                    t_tl = bl; t_tr = tl; t_bl = br; t_br = tr;
                } else if (rotation == 2) { // 180 CW
                    t_tl = br; t_tr = bl; t_bl = tr; t_br = tl;
                } else { // 270 CW
                    t_tl = tr; t_tr = br; t_bl = tl; t_br = bl;
                }

				// Texture Coords: Triangle 1
				triangles[0].texCoords = t_tl; // Top-Left
				triangles[1].texCoords = t_tr; // Top-Right
				triangles[2].texCoords = t_bl; // Bottom-Left

				// Texture Coords: Triangle 2
				triangles[3].texCoords = t_bl; // Bottom-Left
				triangles[4].texCoords = t_tr; // Top-Right
				triangles[5].texCoords = t_br; // Bottom-Right

			}
		}
		return true;
	}

	void MapRenderer::draw(sf::RenderTarget& target, sf::RenderStates states) const {
		states.transform *= getTransform();
		states.texture = &m_tileset;
		target.draw(m_vertices, states);
	}