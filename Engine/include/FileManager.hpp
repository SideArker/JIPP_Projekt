#pragma once

#include "EngineAPI.hpp"
#include "MapFile.hpp"
#include "GameState.hpp"
#include <string>

class ENGINE_API FileManager {
public:
    static bool saveMap (const MapFile&,   const std::string& path);
    static bool loadMap (const std::string& path, MapFile&    out);
    static bool saveGame(const GameState&, const std::string& path);
    static bool loadGame(const std::string& path, GameState&  out);
};
