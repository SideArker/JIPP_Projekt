	#include "MapRenderer.hpp"
	#include "MapManager.hpp"
	#include "SelectionController.hpp"
	#include "TeamRegistry.hpp"
	#include "TextureManager.hpp"
	#include "Building.hpp"
	#include "Unit.hpp"
	#include <iostream>
	#include <cmath>
	#include <algorithm>

static constexpr float kCaptureJump1Duration = 0.34f;
static constexpr float kCaptureJump2Duration = 0.22f;
static constexpr float kCaptureJump3Duration = 0.15f;

static constexpr float kCaptureJump1Amplitude = 13.0f;
static constexpr float kCaptureJump2Amplitude = 7.0f;
static constexpr float kCaptureJump3Amplitude = 3.5f;

static constexpr float kPi = 3.14159265f;

float getCaptureBounceOffsetY(float remainingTime) {
  if (remainingTime <= 0.f || MapRenderer::kCaptureBounceDuration <= 0.f) {
    return 0.f;
  }

  const float elapsed = std::clamp(MapRenderer::kCaptureBounceDuration - remainingTime, 0.f,
                                   MapRenderer::kCaptureBounceDuration);

  if (elapsed < kCaptureJump1Duration) {
    return -kCaptureJump1Amplitude *
           std::sin(kPi * elapsed / kCaptureJump1Duration);
  }

  if (elapsed < (kCaptureJump1Duration + kCaptureJump2Duration)) {
    const float local = elapsed - kCaptureJump1Duration;
    return -kCaptureJump2Amplitude *
           std::sin(kPi * local / kCaptureJump2Duration);
  }

  const float local = elapsed - kCaptureJump1Duration - kCaptureJump2Duration;
  return -kCaptureJump3Amplitude *
         std::sin(kPi * local / kCaptureJump3Duration);
}

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

				float inset = 0.01f;
				float tu0 = tu * tileSize.x + inset;
				float tv0 = tv * tileSize.y + inset;
				float tu1 = (tu + 1) * tileSize.x - inset;
				float tv1 = (tv + 1) * tileSize.y - inset;

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

void MapRenderer::drawScene(sf::RenderTarget& target, const MapManager& mapManager) const {
  target.draw(*this);

  // Draw Game Border (Black and Orange striped)
  float bw = static_cast<float>(mapManager.mapWidth * mapManager.tileSize.x);
  float bh = static_cast<float>(mapManager.mapHeight * mapManager.tileSize.y);
  float t = 4.f;       // border thickness
  float segLen = 16.f; // stripe segment length

  auto addRect = [&](float x, float y, float w, float h, sf::Color c) {
    sf::RectangleShape rect({w, h});
    rect.setPosition({x, y});
    rect.setFillColor(c);
    target.draw(rect);
  };

  // Top and Bottom edges
  for (float x = 0; x < bw; x += segLen) {
    sf::Color c = (static_cast<int>(x / segLen) % 2 == 0)
                      ? sf::Color::Black
                      : sf::Color(255, 128, 0);
    float w = std::min(segLen, bw - x);
    addRect(x, -t, w, t, c); // Top
    addRect(x, bh, w, t, c); // Bottom
  }
  // Left and Right edges
  for (float y = 0; y < bh; y += segLen) {
    sf::Color c = (static_cast<int>(y / segLen) % 2 == 0)
                      ? sf::Color::Black
                      : sf::Color(255, 128, 0);
    float h = std::min(segLen, bh - y);
    addRect(-t, y, t, h, c); // Left
    addRect(bw, y, t, h, c); // Right
  }

  for (const auto &building : mapManager.buildings) {
    sf::Sprite sprite(building->getTexture());
    if (building->hasTextureRect())
      sprite.setTextureRect(building->getTextureRect());
    sprite.setPosition(building->getPosition());
    target.draw(sprite);
  }
  if (mapManager.selectionController)
    mapManager.selectionController->drawOverlays(target);

  bool acting = mapManager.isAnyUnitActing();
  for (const auto &unit : mapManager.units) {
    if (unit->isDead())
      continue;

    sf::Sprite unitSprite(unit->getCurrentTexture());
    sf::IntRect rect = unit->getCurrentRect();
    unitSprite.setTextureRect(rect);
    
    sf::Color color = sf::Color::White;
    if (unit->hasActed() && !unit->isActing()) {
        color = sf::Color(150, 150, 150);
    }
    color.a = static_cast<std::uint8_t>(unit->getSpawnFadeAlpha() * 255.f);
    unitSprite.setColor(color);

    if (unit->shouldFlipX()) {
      unitSprite.setScale({-1.f, 1.f});
      unitSprite.setPosition(
          {unit->getPosition().x + static_cast<float>(rect.size.x),
           unit->getPosition().y});
    } else {
      unitSprite.setPosition(unit->getPosition());
    }
    target.draw(unitSprite);

    if (mapManager.m_unitRenderCallback)
      mapManager.m_unitRenderCallback(target, *unit, acting);
  }

  if (!mapManager.m_teamCaptureTexturePath.empty()) {
    static constexpr float kOverlayOffY = -5.f;
    static constexpr int kCellW = 32;

    for (const auto &building : mapManager.buildings) {
      int progress = building->getCaptureProgress();
      if (progress <= 0)
        continue;

      float bounceY = 0.f;
      if (mapManager.m_captureBounceTimer > 0.f &&
          mapManager.m_captureBounceTargets.find(building.get()) !=
              mapManager.m_captureBounceTargets.end()) {
        bounceY = getCaptureBounceOffsetY(mapManager.m_captureBounceTimer);
      }

      sf::Vector2i bGrid(
          static_cast<int>(std::round(building->getPosition().x /
                                      static_cast<float>(mapManager.tileSize.x))),
          static_cast<int>(std::round(building->getPosition().y /
                                      static_cast<float>(mapManager.tileSize.y))));
      auto occupant = mapManager.getUnitAtTile(bGrid);
      if (!occupant || occupant->isDead())
        continue;

      sf::IntRect rect = occupant->getCurrentRect();
      sf::Sprite bouncedUnit(occupant->getCurrentTexture());
      bouncedUnit.setTextureRect(rect);
      if (occupant->hasActed() && !occupant->isActing())
        bouncedUnit.setColor(sf::Color(150, 150, 150));
      const float ux = occupant->getPosition().x;
      const float uy = occupant->getPosition().y + bounceY;
      if (occupant->shouldFlipX()) {
        bouncedUnit.setScale({-1.f, 1.f});
        bouncedUnit.setPosition({ux + static_cast<float>(rect.size.x), uy});
      } else {
        bouncedUnit.setPosition({ux, uy});
      }
      target.draw(bouncedUnit);

      const sf::Color teamColor =
          TeamRegistry::getColor(building->getCaptureTeam());
      const sf::Texture &capTex = TextureManager::getTexture(
          mapManager.m_teamCaptureTexturePath, mapManager.m_teamCaptureMaskPath, teamColor);

      sf::Sprite capSprite(capTex);
      const int cellIdx = std::clamp(progress - 1, 0, 2);
      capSprite.setTextureRect(
          sf::IntRect({cellIdx * kCellW, 0}, {kCellW, kCellW}));

      const float tx = building->getPosition().x;
      const float ty = building->getPosition().y;

      const float capX =
          tx +
          (static_cast<float>(mapManager.tileSize.x) - static_cast<float>(kCellW)) * 0.5f;
      capSprite.setPosition({capX, ty + kOverlayOffY + bounceY});
      target.draw(capSprite);
    }
  }

  for (const auto &e : mapManager.m_effects)
    target.draw(e.sprite);
  if (mapManager.selectionController)
    mapManager.selectionController->drawCursorIcon(target);
}
void MapRenderer::drawUI(sf::RenderWindow &window, const MapManager& mapManager) const {
  if (!mapManager.m_gameOver) {
    auto td = mapManager.getTeamData(mapManager.m_turnController.getCurrentTeam());
    if (td && td->isAi) {
      sf::View savedView = window.getView();
      window.setView(window.getDefaultView());

      sf::Text text(mapManager.m_uiFont, "ENEMY TURN", 40u);

      float alpha = (std::sin(mapManager.m_uiTime * 4.f) + 1.f) * 0.5f * 255.f;
      sf::Color tcol = td->color;
      tcol.a = static_cast<std::uint8_t>(alpha);
      text.setFillColor(tcol);
      text.setOutlineColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alpha)));
      text.setOutlineThickness(2.f);

      sf::FloatRect bounds = text.getLocalBounds();
      text.setOrigin({bounds.size.x / 1.5f, bounds.size.y / 2.f});
      text.setPosition({window.getSize().x / 2.f, 60.f});

      window.draw(text);
      window.setView(savedView);
    }
  }
}