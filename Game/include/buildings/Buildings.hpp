#pragma once

#include "Building.hpp"

class HQ : public Building {
public:
    HQ(Team team, const BuildingData& data) : Building("HQ", team, data) {}
    void onClicked() override;
};

class Factory : public Building {
public:
    Factory(Team team, const BuildingData& data) : Building("Factory", team, data) {}
    void onClicked() override;
};

class Port : public Building {
public:
    Port(Team team, const BuildingData& data) : Building("Port", team, data) {}
    void onClicked() override;
};

class OilRig : public Building {
public:
    OilRig(Team team, const BuildingData& data) : Building("OilRig", team, data) {}
    void onClicked() override;
};

void registerBuildings();
