#include "UnitInstance.hpp"

using namespace OpenVic;

template<UnitType::branch_t Branch>
UnitInstance<Branch>::UnitInstance(std::string_view new_unit_name, _UnitType const& new_unit_type)
  : unit_name { new_unit_name },
	unit_type { new_unit_type },
	organisation { new_unit_type.get_default_organisation() },
	morale { 0 },
	strength { new_unit_type.get_max_strength() } {}

template<UnitType::branch_t Branch>
void UnitInstance<Branch>::set_unit_name(std::string_view new_unit_name) {
	unit_name = new_unit_name;
}

template struct OpenVic::UnitInstance<UnitType::branch_t::LAND>;
template struct OpenVic::UnitInstance<UnitType::branch_t::NAVAL>;

UnitInstanceBranched<UnitType::branch_t::LAND>::UnitInstanceBranched(
	std::string_view new_name, RegimentType const& new_regiment_type, Pop* new_pop
) : UnitInstance { new_name, new_regiment_type }, pop { new_pop } {}

UnitInstanceBranched<UnitType::branch_t::NAVAL>::UnitInstanceBranched(
	std::string_view new_name, ShipType const& new_ship_type
) : UnitInstance { new_name, new_ship_type } {}
