#include "BuildingRegistry.hpp"

std::unordered_map<std::string, BuildingRegistry::Entry>& BuildingRegistry::registry() {
    static std::unordered_map<std::string, Entry> instance;
    return instance;
}

void BuildingRegistry::registerType(const std::string& typeName, BuildingData data, FactoryFn factory) {
    registry()[typeName] = { std::move(data), std::move(factory) };
}

std::shared_ptr<Building> BuildingRegistry::create(const std::string& typeName, Team team) {
    auto it = registry().find(typeName);
    if (it == registry().end()) return nullptr;
    return it->second.factory(team);
}

const BuildingData* BuildingRegistry::getData(const std::string& typeName) {
    auto it = registry().find(typeName);
    if (it == registry().end()) return nullptr;
    return &it->second.data;
}
