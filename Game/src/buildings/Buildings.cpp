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

void HQ::onClicked() {}
void Airport::onClicked() {}
void Factory::onClicked() {}
void VehicleBase::onClicked() {}
void LandOilRig::onClicked() {}
void Port::onClicked() {}
void SeaOilRig::onClicked() {}

static std::shared_ptr<Building> makeBuilding(const std::string& type, const BuildingData& data, Team team) {
    if (type == "HQ")          return std::make_shared<HQ>(team, data);
    if (type == "Airport")     return std::make_shared<Airport>(team, data);
    if (type == "Factory")     return std::make_shared<Factory>(team, data);
    if (type == "VehicleBase") return std::make_shared<VehicleBase>(team, data);
    if (type == "LandOilRig")  return std::make_shared<LandOilRig>(team, data);
    if (type == "Port")        return std::make_shared<Port>(team, data);
    if (type == "SeaOilRig")   return std::make_shared<SeaOilRig>(team, data);
    return nullptr;
}

void registerBuildings() {
    BuildingData hqData = makeSpritesheetBuildingData(0);
    BuildingRegistry::registerType("HQ", hqData, [hqData](Team team) { return makeBuilding("HQ", hqData, team); });

    BuildingData airportData = makeSpritesheetBuildingData(1);
    BuildingRegistry::registerType("Airport", airportData, [airportData](Team team) { return makeBuilding("Airport", airportData, team); });

    BuildingData factoryData = makeSpritesheetBuildingData(2);
    BuildingRegistry::registerType("Factory", factoryData, [factoryData](Team team) { return makeBuilding("Factory", factoryData, team); });

    BuildingData vehicleBaseData = makeSpritesheetBuildingData(3);
    BuildingRegistry::registerType("VehicleBase", vehicleBaseData, [vehicleBaseData](Team team) { return makeBuilding("VehicleBase", vehicleBaseData, team); });

    BuildingData landOilRigData = makeSpritesheetBuildingData(4);
    BuildingRegistry::registerType("LandOilRig", landOilRigData, [landOilRigData](Team team) { return makeBuilding("LandOilRig", landOilRigData, team); });

    BuildingData portData = makeSpritesheetBuildingData(5);
    BuildingRegistry::registerType("Port", portData, [portData](Team team) { return makeBuilding("Port", portData, team); });

    BuildingData seaOilRigData = makeSpritesheetBuildingData(6);
    BuildingRegistry::registerType("SeaOilRig", seaOilRigData, [seaOilRigData](Team team) { return makeBuilding("SeaOilRig", seaOilRigData, team); });
}
