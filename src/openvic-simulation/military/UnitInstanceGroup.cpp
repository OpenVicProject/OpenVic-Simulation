#include "UnitInstanceGroup.hpp"

#include <vector>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/military/Deployment.hpp"

using namespace OpenVic;

MovementInfo::MovementInfo() : path {}, movement_progress {} {}

//TODO: pathfinding logic
MovementInfo::MovementInfo(ProvinceInstance const* starting_province, ProvinceInstance const* target_province)
	: path { starting_province, target_province }, movement_progress { 0 } {}

UnitInstanceGroupBranched<UnitType::branch_t::LAND>::UnitInstanceGroupBranched(
	std::string_view new_name,
	std::vector<RegimentInstance*>&& new_units,
	_Leader* new_leader,
	CountryInstance* new_country
) : UnitInstanceGroup { new_name, std::move(new_units), new_leader, new_country } {}

UnitInstanceGroupBranched<UnitType::branch_t::NAVAL>::UnitInstanceGroupBranched(
	std::string_view new_name,
	std::vector<ShipInstance*>&& new_units,
	_Leader* new_leader,
	CountryInstance* new_country
) : UnitInstanceGroup { new_name, std::move(new_units), new_leader, new_country } {}

bool UnitInstanceManager::generate_regiment(RegimentDeployment const& regiment_deployment, RegimentInstance*& regiment) {
	// TODO - get pop from Province regiment_deployment.get_home()
	RegimentInstance& regiment_instance =
		*regiments.insert({ regiment_deployment.get_name(), regiment_deployment.get_type(), nullptr });

	regiment = &regiment_instance;

	return true;
}

bool UnitInstanceManager::generate_ship(ShipDeployment const& ship_deployment, ShipInstance*& ship) {
	ShipInstance& ship_instance = *ships.insert({ ship_deployment.get_name(), ship_deployment.get_type() });

	ship = &ship_instance;

	return true;
}

bool UnitInstanceManager::generate_army(
	MapInstance& map_instance, CountryInstance& country, ArmyDeployment const& army_deployment
) {
	if (army_deployment.get_regiments().empty()) {
		Logger::error(
			"Trying to generate army \"", army_deployment.get_name(), "\" with no regiments for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	if (army_deployment.get_location() == nullptr) {
		Logger::error(
			"Trying to generate army \"", army_deployment.get_name(), "\" with no location for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	bool ret = true;

	std::vector<RegimentInstance*> army_regiments;

	for (RegimentDeployment const& regiment_deployment : army_deployment.get_regiments()) {
		RegimentInstance* regiment = nullptr;

		ret &= generate_regiment(regiment_deployment, regiment);

		if (regiment != nullptr) {
			army_regiments.push_back(regiment);
		}
	}

	if (army_regiments.empty()) {
		Logger::error(
			"Failed to generate any regiments for army \"", army_deployment.get_name(), "\" for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	ArmyInstance& army_instance = *armies.insert({ army_deployment.get_name(), std::move(army_regiments), nullptr, &country });

	army_instance.set_position(map_instance.get_province_instance_from_const(army_deployment.get_location()));

	return ret;
}

bool UnitInstanceManager::generate_navy(
	MapInstance& map_instance, CountryInstance& country, NavyDeployment const& navy_deployment
) {
	if (navy_deployment.get_ships().empty()) {
		Logger::error(
			"Trying to generate navy \"", navy_deployment.get_name(), "\" with no ships for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	if (navy_deployment.get_location() == nullptr) {
		Logger::error(
			"Trying to generate navy \"", navy_deployment.get_name(), "\" with no location for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	bool ret = true;

	std::vector<ShipInstance*> navy_ships;

	for (ShipDeployment const& ship_deployment : navy_deployment.get_ships()) {
		ShipInstance* ship = nullptr;

		ret &= generate_ship(ship_deployment, ship);

		if (ship != nullptr) {
			navy_ships.push_back(ship);
		}
	}

	if (navy_ships.empty()) {
		Logger::error(
			"Failed to generate any ships for navy \"", navy_deployment.get_name(), "\" for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	NavyInstance& navy_intance = *navies.insert({ navy_deployment.get_name(), std::move(navy_ships), nullptr, &country });

	navy_intance.set_position(map_instance.get_province_instance_from_const(navy_deployment.get_location()));

	return ret;
}

bool UnitInstanceManager::generate_deployment(
	MapInstance& map_instance, CountryInstance& country, Deployment const* deployment
) {
	if (deployment == nullptr) {
		Logger::error("Trying to generate null deployment for ", country.get_identifier());
		return false;
	}

	bool ret = true;

	for (ArmyDeployment const& army_deployment : deployment->get_armies()) {
		ret &= generate_army(map_instance, country, army_deployment);
	}

	for (NavyDeployment const& navy_deployment : deployment->get_navies()) {
		ret &= generate_navy(map_instance, country, navy_deployment);
	}

	for (LeaderBase const& leader : deployment->get_leaders()) {
		ret &= country.add_leader(leader);
	}

	return ret;
}
