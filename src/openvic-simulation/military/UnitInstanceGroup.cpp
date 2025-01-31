#include "UnitInstanceGroup.hpp"

#include <vector>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/military/Deployment.hpp"

using namespace OpenVic;

using enum UnitType::branch_t;

template<UnitType::branch_t Branch>
UnitInstanceGroup<Branch>::UnitInstanceGroup(
	std::string_view new_name, std::vector<_UnitInstance*>&& new_units
) : name { new_name },
	units { std::move(new_units) } {}

template<UnitType::branch_t Branch>
void UnitInstanceGroup<Branch>::update_gamestate() {
	total_organisation = fixed_point_t::_0();
	total_max_organisation = fixed_point_t::_0();
	total_strength = fixed_point_t::_0();
	total_max_strength = fixed_point_t::_0();

	for (_UnitInstance const* unit : units) {
		total_organisation += unit->get_organisation();
		total_max_organisation += unit->get_max_organisation();
		total_strength += unit->get_strength();
		total_max_strength += unit->get_max_strength();
	}
}

template<UnitType::branch_t Branch>
void UnitInstanceGroup<Branch>::tick() {

}

template<UnitType::branch_t Branch>
size_t UnitInstanceGroup<Branch>::get_unit_count() const {
	return units.size();
}

template<UnitType::branch_t Branch>
bool UnitInstanceGroup<Branch>::empty() const {
	return units.empty();
}

template<UnitType::branch_t Branch>
size_t UnitInstanceGroup<Branch>::get_unit_category_count(UnitType::unit_category_t unit_category) const {
	return std::count_if(units.begin(), units.end(), [unit_category](_UnitInstance const* unit) {
		return unit->get_unit_type().get_unit_category() == unit_category;
	});
}

template<UnitType::branch_t Branch>
UnitType const* UnitInstanceGroup<Branch>::get_display_unit_type() const {
	if (units.empty()) {
		return nullptr;
	}

	fixed_point_map_t<UnitType const*> weighted_unit_types;

	for (_UnitInstance const* unit : units) {
		UnitType const& unit_type = unit->get_unit_type();
		weighted_unit_types[&unit_type] += unit_type.get_weighted_value();
	}

	return get_largest_item_tie_break(
		weighted_unit_types,
		[](UnitType const* lhs, UnitType const* rhs) -> bool {
			return lhs->get_weighted_value() < rhs->get_weighted_value();
		}
	)->first;
}

template<UnitType::branch_t Branch>
void UnitInstanceGroup<Branch>::set_name(std::string_view new_name) {
	name = new_name;
}

template<UnitType::branch_t Branch>
bool UnitInstanceGroup<Branch>::set_position(ProvinceInstance* new_position) {
	bool ret = true;

	if (position != new_position) {
		if (position != nullptr) {
			ret &= position->remove_unit_instance_group(*this);
		}

		position = new_position;

		if (position != nullptr) {
			ret &= position->add_unit_instance_group(*this);
		}
	}

	return ret;
}

template<UnitType::branch_t Branch>
bool UnitInstanceGroup<Branch>::set_country(CountryInstance* new_country) {
	bool ret = true;

	if (country != new_country) {
		if (country != nullptr) {
			ret &= country->remove_unit_instance_group(*this);
		}

		country = new_country;

		if (country != nullptr) {
			ret &= country->add_unit_instance_group(*this);
		}
	}

	return ret;
}

template<UnitType::branch_t Branch>
bool UnitInstanceGroup<Branch>::set_leader(_Leader* new_leader) {
	bool ret = true;

	if (leader != new_leader) {
		if (leader != nullptr) {
			if (leader->unit_instance_group == this) {
				leader->unit_instance_group = nullptr;
			} else {
				Logger::error(
					"Mismatch between leader and unit instance group: group ", name, " has leader ",
					leader->get_name(), " but the leader has group ", leader->get_unit_instance_group() != nullptr
						? leader->get_unit_instance_group()->get_name() : "NULL"
				);
				ret = false;
			}
		}

		leader = new_leader;

		if (leader != nullptr) {
			if (leader->unit_instance_group != nullptr) {
				if (leader->unit_instance_group != this) {
					ret &= leader->unit_instance_group->set_leader(nullptr);
				} else {
					Logger::error("Leader ", leader->get_name(), " already leads group ", name, "!");
					ret = false;
				}
			}

			leader->unit_instance_group = static_cast<UnitInstanceGroupBranched<Branch>*>(this);
		}
	}

	return ret;
}

// These values are only needed for the UI, so there's no need to pre-calculate and cache them for every unit on every update.
template<UnitType::branch_t Branch>
fixed_point_t UnitInstanceGroup<Branch>::get_organisation_proportion() const {
	return total_max_organisation != fixed_point_t::_0() ? total_organisation / total_max_organisation : fixed_point_t::_0();
}

template<UnitType::branch_t Branch>
fixed_point_t UnitInstanceGroup<Branch>::get_strength_proportion() const {
	return total_max_strength != fixed_point_t::_0() ? total_strength / total_max_strength : fixed_point_t::_0();
}

template<UnitType::branch_t Branch>
fixed_point_t UnitInstanceGroup<Branch>::get_average_organisation() const {
	return !units.empty() ? total_organisation / static_cast<int32_t>(units.size()) : fixed_point_t::_0();
}

template<UnitType::branch_t Branch>
fixed_point_t UnitInstanceGroup<Branch>::get_average_max_organisation() const {
	return !units.empty() ? total_max_organisation / static_cast<int32_t>(units.size()) : fixed_point_t::_0();
}

template<UnitType::branch_t Branch>
bool UnitInstanceGroup<Branch>::is_moving() const {
	return !path.empty();
}

template<UnitType::branch_t Branch>
ProvinceInstance const* UnitInstanceGroup<Branch>::get_movement_destination_province() const {
	return !path.empty() ? path.back() : nullptr;
}

template<UnitType::branch_t Branch>
Date UnitInstanceGroup<Branch>::get_movement_arrival_date() const {
	// TODO - calculate today + remaining movement cost / unit speed
	return {};
}

template<UnitType::branch_t Branch>
bool UnitInstanceGroup<Branch>::is_in_combat() const {
	// TODO - check if in combat
	return false;
}

template struct OpenVic::UnitInstanceGroup<LAND>;
template struct OpenVic::UnitInstanceGroup<NAVAL>;

UnitInstanceGroupBranched<LAND>::UnitInstanceGroupBranched(
	std::string_view new_name,
	std::vector<RegimentInstance*>&& new_units
) : UnitInstanceGroup { new_name, std::move(new_units) } {}

void UnitInstanceGroupBranched<LAND>::update_gamestate() {
	UnitInstanceGroup<LAND>::update_gamestate();
}

void UnitInstanceGroupBranched<LAND>::tick() {
	UnitInstanceGroup<LAND>::tick();
}

UnitInstanceGroupBranched<NAVAL>::UnitInstanceGroupBranched(
	std::string_view new_name,
	std::vector<ShipInstance*>&& new_units
) : UnitInstanceGroup { new_name, std::move(new_units) } {}

void UnitInstanceGroupBranched<NAVAL>::update_gamestate() {
	UnitInstanceGroup<NAVAL>::update_gamestate();
}

void UnitInstanceGroupBranched<NAVAL>::tick() {
	UnitInstanceGroup<NAVAL>::tick();
}

fixed_point_t UnitInstanceGroupBranched<NAVAL>::get_total_consumed_supply() const {
	fixed_point_t total_consumed_supply = 0;

	for (ShipInstance const* ship : get_units()) {
		total_consumed_supply += ship->get_unit_type().get_supply_consumption_score();
	}

	return total_consumed_supply;
}

template<UnitType::branch_t Branch>
bool UnitInstanceManager::generate_unit_instance(
	UnitDeployment<Branch> const& unit_deployment, UnitInstanceBranched<Branch>*& unit_instance
) {
	unit_instance = &*get_unit_instances<Branch>().insert(
		[&unit_deployment]() -> UnitInstanceBranched<Branch> {
			if constexpr (Branch == LAND) {
				return {
					unit_deployment.get_name(), unit_deployment.get_type(),
					nullptr, // TODO - get pop from Province unit_deployment.get_home()
					false // Not mobilised
				};
			} else if constexpr (Branch == NAVAL) {
				return { unit_deployment.get_name(), unit_deployment.get_type() };
			}
		}()
	);

	return true;
}

template<UnitType::branch_t Branch>
bool UnitInstanceManager::generate_unit_instance_group(
	MapInstance& map_instance, CountryInstance& country, UnitDeploymentGroup<Branch> const& unit_deployment_group
) {
	if (unit_deployment_group.get_units().empty()) {
		Logger::error(
			"Trying to generate unit group \"", unit_deployment_group.get_name(), "\" with no units for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	if (unit_deployment_group.get_location() == nullptr) {
		Logger::error(
			"Trying to generate unit group \"", unit_deployment_group.get_name(), "\" with no location for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	bool ret = true;

	std::vector<UnitInstanceBranched<Branch>*> unit_instances;

	for (UnitDeployment<Branch> const& unit_deployment : unit_deployment_group.get_units()) {
		UnitInstanceBranched<Branch>* unit_instance = nullptr;

		ret &= generate_unit_instance(unit_deployment, unit_instance);

		if (unit_instance != nullptr) {
			unit_instances.push_back(unit_instance);
		}
	}

	if (unit_instances.empty()) {
		Logger::error(
			"Failed to generate any units for unit group \"", unit_deployment_group.get_name(), "\" for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	UnitInstanceGroupBranched<Branch>& unit_instance_group = *get_unit_instance_groups<Branch>().insert({
		unit_deployment_group.get_name(), std::move(unit_instances)
	});

	ret &= unit_instance_group.set_position(
		&map_instance.get_province_instance_from_definition(*unit_deployment_group.get_location())
	);
	ret &= unit_instance_group.set_country(&country);

	if (unit_deployment_group.get_leader_index().has_value()) {
		plf::colony<LeaderBranched<Branch>>& leaders = country.get_leaders<Branch>();
		typename plf::colony<LeaderBranched<Branch>>::iterator it = leaders.begin();

		advance(it, *unit_deployment_group.get_leader_index());

		if (it < leaders.end()) {
			ret &= unit_instance_group.set_leader(&*it);
		} else {
			Logger::error(
				"Invalid leader index ", *unit_deployment_group.get_leader_index(), " for unit group \"",
				unit_deployment_group.get_name(), "\" for country \"", country.get_identifier(), "\""
			);
			ret = false;
		}
	}

	return ret;
}

bool UnitInstanceManager::generate_leader(
	CultureManager const& culture_manager, CountryInstance& country, LeaderBase const& leader
) {
	using enum UnitType::branch_t;

	const auto add_leader = [&culture_manager, &country, &leader]<UnitType::branch_t Branch>() -> void {
		LeaderBranched<Branch> leader_branched { leader };

		if (leader_branched.get_picture().empty() && country.get_primary_culture() != nullptr) {
			leader_branched.set_picture(culture_manager.get_leader_picture_name(
				country.get_primary_culture()->get_group().get_leader(), leader.get_branch()
			));
		}

		country.add_leader(std::move(leader_branched));
	};

	switch (leader.get_branch()) {
	case LAND:
		add_leader.template operator()<LAND>();
		return true;
	case NAVAL:
		add_leader.template operator()<NAVAL>();
		return true;
	default:
		Logger::error(
			"Invalid branch ", static_cast<uint64_t>(leader.get_branch()), " for leader \"", leader.get_name(),
			"\", cannot add to country ", country.get_identifier()
		);
		return false;;
	}
}

bool UnitInstanceManager::generate_deployment(
	CultureManager const& culture_manager, MapInstance& map_instance, CountryInstance& country, Deployment const* deployment
) {
	if (deployment == nullptr) {
		Logger::error("Trying to generate null deployment for ", country.get_identifier());
		return false;
	}

	bool ret = true;

	for (LeaderBase const& leader : deployment->get_leaders()) {
		ret &= generate_leader(culture_manager, country, leader);
	}

	const auto generate_group = [this, &map_instance, &country, &ret, deployment]<UnitType::branch_t Branch>() -> void {
		for (UnitDeploymentGroup<Branch> const& unit_deployment_group : deployment->get_unit_deployment_groups<Branch>()) {
			ret &= generate_unit_instance_group(map_instance, country, unit_deployment_group);
		}
	};

	generate_group.template operator()<LAND>();
	generate_group.template operator()<NAVAL>();

	return ret;
}

void UnitInstanceManager::update_gamestate() {
	for (ArmyInstance& army : armies) {
		army.update_gamestate();
	}
	for (NavyInstance& navy : navies) {
		navy.update_gamestate();
	}
}

void UnitInstanceManager::tick() {
	for (ArmyInstance& army : armies) {
		army.tick();
	}
	for (NavyInstance& navy : navies) {
		navy.tick();
	}
}
