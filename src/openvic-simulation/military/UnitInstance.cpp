#include "UnitInstance.hpp"

using namespace OpenVic;

UnitInstance::UnitInstance(
	unique_id_t new_unique_id,
	std::string_view new_name,
	UnitType const& new_unit_type
) : unique_id { new_unique_id },
	name { new_name },
	unit_type { new_unit_type },
	organisation { unit_type.get_default_organisation() },
	max_organisation { organisation },
	strength { get_max_strength() } {}

void UnitInstance::set_name(std::string_view new_name) {
	name = new_name;
}

UnitInstanceBranched<unit_branch_t::LAND>::UnitInstanceBranched(
	unique_id_t new_unique_id,
	std::string_view new_name,
	RegimentType const& new_regiment_type,
	Pop* new_pop,
	bool new_mobilised
) : UnitInstance { new_unique_id, new_name, new_regiment_type },
	pop { new_pop },
	mobilised { new_mobilised } {}

UnitInstanceBranched<unit_branch_t::NAVAL>::UnitInstanceBranched(
	unique_id_t new_unique_id,
	std::string_view new_name,
	ShipType const& new_ship_type
) : UnitInstance { new_unique_id, new_name, new_ship_type } {}
