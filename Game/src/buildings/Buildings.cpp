#include "buildings/Buildings.hpp"
#include "BuildingRegistry.hpp"

namespace {
constexpr const char* BUILDINGS_SHEET_PATH = "Art/Buildings/buildings.png";
constexpr const char* BUILDINGS_SHEET_MASK = "Art/Buildings/buildings_mask.png";
constexpr int BUILDING_CELL_SIZE = 32;

BuildingData makeSpritesheetBuildingData(int cellIndex, int captureThreshold = 3) {
    BuildingData data;
    data.texturePath = BUILDINGS_SHEET_PATH;
    data.maskPath = BUILDINGS_SHEET_MASK;
    data.useTextureRect = true;
    data.textureRect = sf::IntRect({ cellIndex * BUILDING_CELL_SIZE, 0 }, { BUILDING_CELL_SIZE, BUILDING_CELL_SIZE });
    data.captureThreshold = captureThreshold;
    return data;
}
}

void HQ::onClicked() {
    // TODO: Display HQ info panel; winner/loser determined via onCaptured callback
}

void Factory::onClicked() {
    // TODO: Open ground unit production menu
}

void Port::onClicked() {
    // TODO: Open naval unit production menu
}

void OilRig::onClicked() {
    // TODO: Generate cash for the owning team each turn
}

static std::shared_ptr<Building> makeBuilding(const std::string& type, const BuildingData& data, Team team) {
    if (type == "HQ")      return std::make_shared<HQ>(team, data);
    if (type == "Factory") return std::make_shared<Factory>(team, data);
    if (type == "Port")    return std::make_shared<Port>(team, data);
    if (type == "OilRig")  return std::make_shared<OilRig>(team, data);
    return nullptr;
}

void registerBuildings() {
    BuildingData hqData = makeSpritesheetBuildingData(0);
    BuildingRegistry::registerType("HQ", hqData, [hqData](Team team) {
        return makeBuilding("HQ", hqData, team);
    });

    BuildingData factoryData = makeSpritesheetBuildingData(1);
    BuildingRegistry::registerType("Factory", factoryData, [factoryData](Team team) {
        return makeBuilding("Factory", factoryData, team);
    });

    BuildingData portData = makeSpritesheetBuildingData(2);
    BuildingRegistry::registerType("Port", portData, [portData](Team team) {
        return makeBuilding("Port", portData, team);
    });

    BuildingData oilRigData = makeSpritesheetBuildingData(3);
    BuildingRegistry::registerType("OilRig", oilRigData, [oilRigData](Team team) {
        return makeBuilding("OilRig", oilRigData, team);
    });
}
