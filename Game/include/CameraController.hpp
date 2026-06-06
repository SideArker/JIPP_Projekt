#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

class Unit;

class CameraController {
public:
  void init(sf::View view, sf::Vector2u mapSizePixels, sf::Vector2u windowSize);
  void update(float dt, const sf::RenderWindow &window);

  void trackUnit(std::shared_ptr<const Unit> unit);
  void release();

  const sf::View &getView() const { return m_view; }

private:
  void applyPlayerScroll(float dt, const sf::RenderWindow &window);
  void clampView();

  sf::View m_view;
  sf::Vector2f m_mapBoundsMax;

  std::weak_ptr<const Unit> m_trackedUnit;
  bool m_locked = false;

  static constexpr float kScrollSpeed = 200.f;
  static constexpr float kLerpSpeed = 12.f;
  static constexpr float kEdgeThreshold = 20.f;
};
