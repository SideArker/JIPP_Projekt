#include "UnitRegistry.hpp"

std::unordered_map<std::string, UnitRegistry::Entry>& UnitRegistry::registry() {
    static std::unordered_map<std::string, Entry> instance;
    return instance;
}

void UnitRegistry::registerType(const std::string& typeName, UnitData data, FactoryFn factory) {
    registry()[typeName] = { std::move(data), std::move(factory) };
}

std::shared_ptr<Unit> UnitRegistry::create(const std::string& typeName, Team team) {
    auto it = registry().find(typeName);
    if (it == registry().end()) return nullptr;
    return it->second.factory(team);
}

const UnitData* UnitRegistry::getData(const std::string& typeName) {
    auto it = registry().find(typeName);
    if (it == registry().end()) return nullptr;
    return &it->second.data;
}

std::vector<std::string> UnitRegistry::getRegisteredUnitNames() {
    std::vector<std::string> names;
    for (const auto& pair : registry()) {
        names.push_back(pair.first);
    }
    return names;
}
