#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class TextureManager {
public:
	static sf::Image recolorSpriteMasked(const std::string& artPath, const std::string& maskPath, sf::Color targetColor);

};