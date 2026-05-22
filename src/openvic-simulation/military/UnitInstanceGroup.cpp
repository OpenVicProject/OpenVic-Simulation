#include "UnitInstanceGroup.hpp"

#include <fmt/std.h>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/types/OrderedContainersMath.hpp"

using namespace OpenVic;

using enum unit_branch_t;

UnitInstanceGroup::UnitInstanceGroup(
	unique_id_t new_unique_id,
	unit_branch_t new_branch,
	std::string_view new_name,
	CountryInstance& new_country,
	ProvinceInstance& new_location
) : unique_id { new_unique_id },
	branch { new_branch },
	name { new_name },
	country { new_country },
	location { new_location } {
		new_country.add_unit_instance_group(*this);
	}

void UnitInstanceGroup::update_gamestate() {
	total_organisation = 0;
	total_max_organisation = 0;
	total_strength = 0;
	total_max_strength = 0;

	for (UnitInstance const& unit : units) {
		total_organisation += unit.get_organisation();
		total_max_organisation += unit.get_max_organisation();
		total_strength += unit.get_strength();
		total_max_strength += unit.get_max_strength();
	}
}

void UnitInstanceGroup::tick() {

}

size_t UnitInstanceGroup::get_unit_count() const {
	return units.size();
}

bool UnitInstanceGroup::empty() const {
	return units.empty();
}

size_t UnitInstanceGroup::get_unit_category_count(UnitType::unit_category_t unit_category) const {
	return std::count_if(units.begin(), units.end(), [unit_category](UnitInstance const& unit) {
		return unit.unit_type.unit_category == unit_category;
	});
}

UnitType const* UnitInstanceGroup::get_display_unit_type() const {
	if (units.empty()) {
		return nullptr;
	}

	fixed_point_map_t<UnitType const*> weighted_unit_types;

	for (UnitInstance const& unit : units) {
		UnitType const& unit_type = unit.unit_type;
		weighted_unit_types[&unit_type] += unit_type.weighted_value;
	}

	return get_largest_item_tie_break(
		weighted_unit_types,
		[](UnitType const* lhs, UnitType const* rhs) -> bool {
			return lhs->weighted_value < rhs->weighted_value;
		}
	)->first;
}

bool UnitInstanceGroup::add_unit(UnitInstance& unit) {
	if (unit.get_branch() == branch) {
		units.emplace_back(unit);
		return true;
	} else {
		spdlog::error_s(
			"Trying to add {} unit \"{}\" to {} unit group \"{}\"",
			get_branch_name(unit.get_branch()), unit.get_name(), get_branch_name(branch), get_name()
		);
		return false;
	}
}

bool UnitInstanceGroup::remove_unit(UnitInstance const& unit) {
	auto it = std::find(units.begin(), units.end(), unit);

	if (it != units.end()) {
		units.erase(it);
		return true;
	} else {
		spdlog::error_s(
			"Trying to remove non-existent unit \"{}\" from unit group \"{}\"",
			unit.get_name(), get_name()
		);
		return false;
	}
}

void UnitInstanceGroup::set_name(std::string_view new_name) {
	name = new_name;
}

bool UnitInstanceGroup::set_location(ProvinceInstance& new_location) {
	bool ret = true;

	if (location != new_location) {
		ret &= location.get().remove_unit_instance_group(*this);
		location = new_location;
		ret &= new_location.add_unit_instance_group(*this);
	}

	return ret;
}

bool UnitInstanceGroup::set_country(CountryInstance& new_country) {
	bool ret = true;

	if (country != new_country) {
		ret &= country.get().remove_unit_instance_group(*this);
		country = new_country;
		ret &= new_country.add_unit_instance_group(*this);
	}

	return ret;
}

bool UnitInstanceGroup::set_leader(LeaderInstance* new_leader) {
	bool ret = true;

	if (leader != new_leader) {
		if (leader != nullptr) {
			if (leader->unit_instance_group == this) {
				leader->unit_instance_group = nullptr;
			} else {
				spdlog::error_s(
					"Mismatch between leader and unit instance group: group {} has leader {} but the leader has group {}",
					name,
					leader->get_name(),
					leader->get_unit_instance_group() != nullptr ? leader->get_unit_instance_group()->get_name() : "NULL"
				);
				ret = false;
			}
		}

		leader = nullptr;

		if (new_leader != nullptr) {
			if (OV_unlikely(new_leader->branch != branch)) {
				spdlog::error_s(
					"Trying to assign {} leader \"{}\" to {} unit group \"{}\"",
					get_branch_name(new_leader->branch), new_leader->get_name(), get_branch_name(branch), name
				);
				return false;
			}

			if (OV_unlikely(new_leader->country != country)) {
				spdlog::error_s(
					"Trying to assign {} \"{}\" of country \"{}\" to {} \"{}\" of country \"{}\"",
					get_branched_leader_name(new_leader->branch),
					new_leader->get_name(),
					new_leader->country,
					get_branched_unit_group_name(branch),
					name,
					country
				);
				return false;
			}

			if (new_leader->unit_instance_group != nullptr) {
				if (new_leader->unit_instance_group != this) {
					ret &= new_leader->unit_instance_group->set_leader(nullptr);
				} else {
					spdlog::error_s(
						"{} {} already leads {} {}!",
						get_branched_leader_name(new_leader->branch), new_leader->get_name(),get_branched_unit_group_name(branch), name
					);
					ret = false;
				}
			}

			leader = new_leader;
			leader->unit_instance_group = this;
		}
	}

	return ret;
}

// These values are only needed for the UI, so there's no need to pre-calculate and cache them for every unit on every update.
fixed_point_t UnitInstanceGroup::get_organisation_proportion() const {
	return total_max_organisation != 0 ? total_organisation / total_max_organisation : fixed_point_t::_0;
}

fixed_point_t UnitInstanceGroup::get_strength_proportion() const {
	return total_max_strength != 0 ? total_strength / total_max_strength : fixed_point_t::_0;
}

fixed_point_t UnitInstanceGroup::get_average_organisation() const {
	return !units.empty() ? total_organisation / static_cast<int32_t>(units.size()) : fixed_point_t::_0;
}

fixed_point_t UnitInstanceGroup::get_average_max_organisation() const {
	return !units.empty() ? total_max_organisation / static_cast<int32_t>(units.size()) : fixed_point_t::_0;
}

bool UnitInstanceGroup::is_moving() const {
	return !path.empty();
}

ProvinceInstance const* UnitInstanceGroup::get_movement_destination_province() const {
	return !path.empty() ? &path.back().get() : nullptr;
}

Date UnitInstanceGroup::get_movement_arrival_date() const {
	// TODO - calculate today + remaining movement cost / unit speed
	return {};
}

bool UnitInstanceGroup::is_in_combat() const {
	// TODO - check if in combat
	return false;
}

UnitInstanceGroupBranched<LAND>::UnitInstanceGroupBranched(
	unique_id_t new_unique_id,
	std::string_view new_name,
	CountryInstance& new_country,
	ProvinceInstance& new_location
) : UnitInstanceGroup { new_unique_id, LAND, new_name, new_country, new_location } {}

void UnitInstanceGroupBranched<LAND>::update_gamestate() {
	UnitInstanceGroup::update_gamestate();
}

void UnitInstanceGroupBranched<LAND>::tick() {
	UnitInstanceGroup::tick();
}

UnitInstanceGroupBranched<NAVAL>::UnitInstanceGroupBranched(
	unique_id_t new_unique_id,
	std::string_view new_name,
	CountryInstance& new_country,
	ProvinceInstance& new_location
) : UnitInstanceGroup { new_unique_id, NAVAL, new_name, new_country, new_location } {}

void UnitInstanceGroupBranched<NAVAL>::update_gamestate() {
	UnitInstanceGroup::update_gamestate();
}

void UnitInstanceGroupBranched<NAVAL>::tick() {
	UnitInstanceGroup::tick();
}

fixed_point_t UnitInstanceGroupBranched<NAVAL>::get_total_consumed_supply() const {
	fixed_point_t total_consumed_supply = 0;

	for (ShipInstance const& ship : get_ship_instances()) {
		total_consumed_supply += ship.get_ship_type().supply_consumption_score;
	}

	return total_consumed_supply;
}
