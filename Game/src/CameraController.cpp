#include "CameraController.hpp"
#include "Unit.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>

void CameraController::init(sf::View view, sf::Vector2u mapSizePixels, sf::Vector2u windowSize) {
    m_view         = view;
    m_mapBoundsMax = sf::Vector2f(static_cast<float>(mapSizePixels.x),
                                  static_cast<float>(mapSizePixels.y));
    clampView();
}

void CameraController::trackUnit(std::shared_ptr<const Unit> unit) {
    m_trackedUnit = unit;
    m_locked      = true;
}

void CameraController::release() {
    m_trackedUnit.reset();
    m_locked = false;
}

void CameraController::update(float dt, const sf::RenderWindow& window) {
    auto tracked = m_trackedUnit.lock();
    if (tracked) {
        sf::Vector2f halfTile(16.f, 16.f);
        sf::Vector2f target = tracked->getPosition() + halfTile;
        sf::Vector2f current = m_view.getCenter();
        m_view.setCenter(current + (target - current) * std::min(1.f, kLerpSpeed * dt));
    } else {
        m_locked = false;
        applyPlayerScroll(dt, window);
    }

    clampView();
}

void CameraController::applyPlayerScroll(float dt, const sf::RenderWindow& window) {
    sf::Vector2f delta;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) delta.y -= kScrollSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) delta.y += kScrollSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) delta.x -= kScrollSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) delta.x += kScrollSpeed * dt;

    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2u winSize  = window.getSize();
    float wx = static_cast<float>(winSize.x);
    float wy = static_cast<float>(winSize.y);
    float mx = static_cast<float>(mousePos.x);
    float my = static_cast<float>(mousePos.y);

    if (mx >= 0.f && mx < wx && my >= 0.f && my < wy) {
        if (mx < kEdgeThreshold)        delta.x -= kScrollSpeed * dt;
        if (mx > wx - kEdgeThreshold)   delta.x += kScrollSpeed * dt;
        if (my < kEdgeThreshold)        delta.y -= kScrollSpeed * dt;
        if (my > wy - kEdgeThreshold)   delta.y += kScrollSpeed * dt;
    }

    m_view.move(delta);
}

void CameraController::clampView() {
    sf::Vector2f half = m_view.getSize() / 2.f;
    sf::Vector2f c    = m_view.getCenter();

    c.x = std::clamp(c.x, half.x, std::max(half.x, m_mapBoundsMax.x - half.x));
    c.y = std::clamp(c.y, half.y, std::max(half.y, m_mapBoundsMax.y - half.y));

    m_view.setCenter(c);
}
