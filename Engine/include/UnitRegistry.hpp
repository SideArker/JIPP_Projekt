#pragma once

#include "EngineAPI.hpp"
#include "Unit.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class ENGINE_API UnitRegistry {
public:
    using FactoryFn = std::function<std::shared_ptr<Unit>(Team)>;

    static void registerType(const std::string& typeName, UnitData data, FactoryFn factory);
    static std::shared_ptr<Unit> create(const std::string& typeName, Team team);
    static const UnitData* getData(const std::string& typeName);

private:
    struct Entry {
        UnitData data;
        FactoryFn factory;
    };
    static std::unordered_map<std::string, Entry>& registry();
};
