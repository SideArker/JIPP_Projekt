#pragma once

#include "EngineAPI.hpp"
#include "Building.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class ENGINE_API BuildingRegistry {
public:
    using FactoryFn = std::function<std::shared_ptr<Building>(Team)>;

    static void registerType(const std::string& typeName, BuildingData data, FactoryFn factory);
    static std::shared_ptr<Building> create(const std::string& typeName, Team team);
    static const BuildingData* getData(const std::string& typeName);

private:
    struct Entry {
        BuildingData data;
        FactoryFn    factory;
    };
    static std::unordered_map<std::string, Entry>& registry();
};
