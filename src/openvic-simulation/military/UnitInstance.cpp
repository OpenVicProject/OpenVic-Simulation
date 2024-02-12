#include "UnitInstance.hpp"
#include <vector>
#include "openvic-simulation/military/UnitType.hpp"

using namespace OpenVic;

RegimentInstance::RegimentInstance(std::string_view new_name, RegimentType const& new_regiment_type, Pop& new_pop)
	: UnitInstance { new_name, new_regiment_type }, pop { new_pop } {}

ShipInstance::ShipInstance(std::string_view new_name, ShipType const& new_ship_type)
	: UnitInstance { new_name, new_ship_type } {}

MovementInfo::MovementInfo() : path {}, movement_progress {} {}

//TODO: pathfinding logic
MovementInfo::MovementInfo(Province const* starting_province, Province const* target_province)
	: path { std::vector { starting_province, target_province } }, movement_progress { 0 } {}

ArmyInstance::ArmyInstance(
	std::string_view new_name,
	std::vector<RegimentInstance*>&& new_units,
	Leader const* new_leader,
	Province const* new_position
) : UnitInstanceGroup { new_name, UnitType::branch_t::LAND, std::move(new_units), new_leader, new_position } {}

NavyInstance::NavyInstance(
	std::string_view new_name,
	std::vector<ShipInstance*>&& new_units,
	Leader const* new_leader,
	Province const* new_position
) : UnitInstanceGroup { new_name, UnitType::branch_t::NAVAL, std::move(new_units), new_leader, new_position } {}