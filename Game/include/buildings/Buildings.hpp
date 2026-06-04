#pragma once

#include "Building.hpp"

class HQ : public Building {
public:
    HQ(Team team, const BuildingData& data) : Building("HQ", team, data) {}
    void onClicked() override;
};

class Airport : public Building {
public:
    Airport(Team team, const BuildingData& data) : Building("Airport", team, data) {}
    void onClicked() override;
};

class Factory : public Building {
public:
    Factory(Team team, const BuildingData& data) : Building("Factory", team, data) {}
    void onClicked() override;
};

class VehicleBase : public Building {
public:
    VehicleBase(Team team, const BuildingData& data) : Building("VehicleBase", team, data) {}
    void onClicked() override;
};

class LandOilRig : public Building {
public:
    LandOilRig(Team team, const BuildingData& data) : Building("LandOilRig", team, data) {}
    void onClicked() override;
};

class Port : public Building {
public:
    Port(Team team, const BuildingData& data) : Building("Port", team, data) {}
    void onClicked() override;
};

class SeaOilRig : public Building {
public:
    SeaOilRig(Team team, const BuildingData& data) : Building("SeaOilRig", team, data) {}
    void onClicked() override;
};

void registerBuildings();
