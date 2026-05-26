#pragma once

#include "EngineAPI.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <cstdint>
#include <map>

class ENGINE_API TextureManager {
private:
    struct CacheKey {
        std::string artPath, maskPath;
        std::uint32_t color;
        bool operator<(const CacheKey& o) const {
            if (artPath != o.artPath) return artPath < o.artPath;
            if (maskPath != o.maskPath) return maskPath < o.maskPath;
            return color < o.color;
        }
    };

    static std::map<CacheKey, sf::Texture> cache;
    static sf::Image recolorSpriteMasked(const std::string& artPath, const std::string& maskPath, sf::Color color);

public:
    static void clearCache();
    static const sf::Texture& getTexture(const std::string& artPath, const std::string& maskPath, sf::Color teamColor);
};