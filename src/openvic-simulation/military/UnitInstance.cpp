#include "UnitInstance.hpp"

using namespace OpenVic;

UnitInstanceBranched<UnitType::branch_t::LAND>::UnitInstanceBranched(
	std::string_view new_name, RegimentType const& new_regiment_type, Pop* new_pop
) : UnitInstance { new_name, new_regiment_type }, pop { new_pop } {}

UnitInstanceBranched<UnitType::branch_t::NAVAL>::UnitInstanceBranched(
	std::string_view new_name, ShipType const& new_ship_type
) : UnitInstance { new_name, new_ship_type } {}
