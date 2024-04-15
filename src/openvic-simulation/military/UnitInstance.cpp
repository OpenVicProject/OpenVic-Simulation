#include "UnitInstance.hpp"

#include <vector>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/Map.hpp"
#include "openvic-simulation/military/UnitType.hpp"

using namespace OpenVic;

RegimentInstance::RegimentInstance(std::string_view new_name, RegimentType const& new_regiment_type, Pop* new_pop)
	: UnitInstance { new_name, new_regiment_type }, pop { new_pop } {}

ShipInstance::ShipInstance(std::string_view new_name, ShipType const& new_ship_type)
	: UnitInstance { new_name, new_ship_type } {}

MovementInfo::MovementInfo() : path {}, movement_progress {} {}

//TODO: pathfinding logic
MovementInfo::MovementInfo(Province const* starting_province, Province const* target_province)
	: path { starting_province, target_province }, movement_progress { 0 } {}

ArmyInstance::ArmyInstance(
	std::string_view new_name,
	std::vector<RegimentInstance*>&& new_units,
	Leader const* new_leader,
	CountryInstance* new_country
) : UnitInstanceGroup { new_name, UnitType::branch_t::LAND, std::move(new_units), new_leader, new_country } {}

void ArmyInstance::set_position(Province* new_position) {
	if (position != new_position) {
		if (position != nullptr) {
			position->remove_army(*this);
		}
		position = new_position;
		if (position != nullptr) {
			position->add_army(*this);
		}
	}
}

NavyInstance::NavyInstance(
	std::string_view new_name,
	std::vector<ShipInstance*>&& new_units,
	Leader const* new_leader,
	CountryInstance* new_country
) : UnitInstanceGroup { new_name, UnitType::branch_t::NAVAL, std::move(new_units), new_leader, new_country } {}

void NavyInstance::set_position(Province* new_position) {
	if (position != new_position) {
		if (position != nullptr) {
			position->remove_navy(*this);
		}
		position = new_position;
		if (position != nullptr) {
			position->add_navy(*this);
		}
	}
}

bool UnitInstanceManager::generate_regiment(RegimentDeployment const& regiment_deployment, RegimentInstance*& regiment) {
	// TODO - get pop from Province regiment_deployment.get_home()
	regiments.push_back({ regiment_deployment.get_name(), regiment_deployment.get_type(), nullptr });

	regiment = &regiments.back();

	return true;
}

bool UnitInstanceManager::generate_ship(ShipDeployment const& ship_deployment, ShipInstance*& ship) {
	ships.push_back({ ship_deployment.get_name(), ship_deployment.get_type() });

	ship = &ships.back();

	return true;
}

bool UnitInstanceManager::generate_army(Map& map, CountryInstance& country, ArmyDeployment const& army_deployment) {
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

	armies.push_back({ army_deployment.get_name(), std::move(army_regiments), nullptr, &country });

	armies.back().set_position(map.remove_province_const(army_deployment.get_location()));

	return ret;
}

bool UnitInstanceManager::generate_navy(Map& map, CountryInstance& country, NavyDeployment const& navy_deployment) {
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

	navies.push_back({ navy_deployment.get_name(), std::move(navy_ships), nullptr, &country });

	navies.back().set_position(map.remove_province_const(navy_deployment.get_location()));

	return ret;
}

bool UnitInstanceManager::generate_deployment(Map& map, CountryInstance& country, Deployment const* deployment) {
	if (deployment == nullptr) {
		Logger::error("Trying to generate null deployment for ", country.get_identifier());
		return false;
	}

	// TODO - Leaders (could be stored in CountryInstance?)

	bool ret = true;

	for (ArmyDeployment const& army_deployment : deployment->get_armies()) {
		ret &= generate_army(map, country, army_deployment);
	}

	for (NavyDeployment const& navy_deployment : deployment->get_navies()) {
		ret &= generate_navy(map, country, navy_deployment);
	}

	return ret;
}
