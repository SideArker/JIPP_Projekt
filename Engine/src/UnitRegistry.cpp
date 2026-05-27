#include "UnitRegistry.hpp"

std::unordered_map<std::string, UnitRegistry::FactoryFn>& UnitRegistry::registry() {
    static std::unordered_map<std::string, FactoryFn> instance;
    return instance;
}

void UnitRegistry::registerType(const std::string& typeName, FactoryFn factory) {
    registry()[typeName] = std::move(factory);
}

std::shared_ptr<Unit> UnitRegistry::create(const std::string& typeName, Team team) {
    auto it = registry().find(typeName);
    if (it == registry().end()) return nullptr;
    return it->second(team);
}
