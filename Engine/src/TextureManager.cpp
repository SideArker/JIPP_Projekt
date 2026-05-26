#include "TextureManager.hpp"
#include <algorithm>
#include <stdexcept>
#include <cstdint>

std::map<TextureManager::CacheKey, sf::Texture> TextureManager::cache;

const sf::Texture& TextureManager::getTexture(const std::string& artPath, const std::string& maskPath, sf::Color teamColor) {
    std::uint32_t packed = (static_cast<std::uint32_t>(teamColor.r) << 24) |
                           (static_cast<std::uint32_t>(teamColor.g) << 16) |
                           (static_cast<std::uint32_t>(teamColor.b) <<  8) |
                            static_cast<std::uint32_t>(teamColor.a);
    CacheKey key{ artPath, maskPath, packed };
    auto [it, inserted] = cache.try_emplace(key);
    if (inserted) {
        sf::Image image = recolorSpriteMasked(artPath, maskPath, teamColor);
        if (!it->second.loadFromImage(image))
            throw std::runtime_error("Failed to load texture: " + artPath);
    }
    return it->second;
}

sf::Image TextureManager::recolorSpriteMasked(const std::string& artPath, const std::string& maskPath, sf::Color targetColor) {
	sf::Image baseImage, maskImage;

	if (!baseImage.loadFromFile(artPath)) {
		throw std::runtime_error("Failed to load art image: " + artPath);
	}

	if (!maskImage.loadFromFile(maskPath)) {
		throw std::runtime_error("Failed to load mask image: " + maskPath);
	}

	if (baseImage.getSize() != maskImage.getSize()) {
		throw std::runtime_error("Art and mask images must be the same size.");
	}

	sf::Vector2u imageSize = baseImage.getSize();
	sf::Image resultImage;

	resultImage.resize(imageSize, sf::Color::Transparent);

	float tr = targetColor.r / 255.0f;
	float tg = targetColor.g / 255.0f;
	float tb = targetColor.b / 255.0f;

	for (unsigned int y = 0; y < imageSize.y; ++y) {
		for (unsigned int x = 0; x < imageSize.x; ++x)
		{
			sf::Color basePixel = baseImage.getPixel({ x, y });
			sf::Color maskPixel = maskImage.getPixel({ x, y });

			// Skip transparent
			if (basePixel.a == 0) continue;

			float gray = (0.299f * basePixel.r + 0.587f * basePixel.g + 0.114f * basePixel.b) / 255.0f;
			float r, g, b;

			if (gray < 0.5f) {
				r = 2.0f * gray * tr;
				g = 2.0f * gray * tg;
				b = 2.0f * gray * tb;
			}
			else {
				r = 1.0f - 2.0f * (1.0f - gray) * (1.0f - tr);
				g = 1.0f - 2.0f * (1.0f - gray) * (1.0f - tg);
				b = 1.0f - 2.0f * (1.0f - gray) * (1.0f - tb);
			}

			sf::Color recoloredPixel;
			recoloredPixel.r = static_cast<std::uint8_t>(std::clamp(r * 255.0f, 0.0f, 255.0f));
			recoloredPixel.g = static_cast<std::uint8_t>(std::clamp(g * 255.0f, 0.0f, 255.0f));
			recoloredPixel.b = static_cast<std::uint8_t>(std::clamp(b * 255.0f, 0.0f, 255.0f));

			float maskIntensity = maskPixel.r / 255.0f;

			sf::Color finalPixel;
			finalPixel.r = static_cast<std::uint8_t>((1.0f - maskIntensity) * basePixel.r + maskIntensity * recoloredPixel.r);
			finalPixel.g = static_cast<std::uint8_t>((1.0f - maskIntensity) * basePixel.g + maskIntensity * recoloredPixel.g);
			finalPixel.b = static_cast<std::uint8_t>((1.0f - maskIntensity) * basePixel.b + maskIntensity * recoloredPixel.b);
			finalPixel.a = basePixel.a;

			resultImage.setPixel({ x, y }, finalPixel);
		}
	}
	return resultImage;
}

void TextureManager::clearCache() {
	cache.clear();
}