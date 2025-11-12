#include "UnitInstanceGroup.hpp"

#include <vector>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/defines/MilitaryDefines.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/military/Deployment.hpp"
#include "openvic-simulation/military/LeaderTrait.hpp"
#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/types/OrderedContainersMath.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/FormatValidate.hpp"

using namespace OpenVic;

using enum unit_branch_t;

UnitInstanceGroup::UnitInstanceGroup(
	unique_id_t new_unique_id,
	unit_branch_t new_branch,
	std::string_view new_name
) : unique_id { new_unique_id },
	branch { new_branch },
	name { new_name } {}

void UnitInstanceGroup::update_gamestate() {
	total_organisation = 0;
	total_max_organisation = 0;
	total_strength = 0;
	total_max_strength = 0;

	for (UnitInstance const* unit : units) {
		total_organisation += unit->get_organisation();
		total_max_organisation += unit->get_max_organisation();
		total_strength += unit->get_strength();
		total_max_strength += unit->get_max_strength();
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
	return std::count_if(units.begin(), units.end(), [unit_category](UnitInstance const* unit) {
		return unit->get_unit_type().get_unit_category() == unit_category;
	});
}

UnitType const* UnitInstanceGroup::get_display_unit_type() const {
	if (units.empty()) {
		return nullptr;
	}

	fixed_point_map_t<UnitType const*> weighted_unit_types;

	for (UnitInstance const* unit : units) {
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

bool UnitInstanceGroup::add_unit(UnitInstance& unit) {
	if (unit.get_branch() == branch) {
		units.push_back(&unit);
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
	const memory::vector<UnitInstance*>::const_iterator it = std::find(units.begin(), units.end(), &unit);

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

bool UnitInstanceGroup::set_position(ProvinceInstance* new_position) {
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

bool UnitInstanceGroup::set_country(CountryInstance* new_country) {
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
			if (OV_unlikely(new_leader->get_branch() != branch)) {
				spdlog::error_s(
					"Trying to assign {} leader \"{}\" to {} unit group \"{}\"",
					get_branch_name(new_leader->get_branch()), new_leader->get_name(), get_branch_name(branch), name
				);
				return false;
			}

			if (OV_unlikely(&new_leader->get_country() != country)) {
				spdlog::error_s(
					"Trying to assign {} \"{}\" of country \"{}\" to {} \"{}\" of country \"{}\"",
					get_branched_leader_name(new_leader->get_branch()),
					new_leader->get_name(),
					new_leader->get_country(),
					get_branched_unit_group_name(branch),
					name,
					ovfmt::validate(country)
				);
				return false;
			}

			if (new_leader->unit_instance_group != nullptr) {
				if (new_leader->unit_instance_group != this) {
					ret &= new_leader->unit_instance_group->set_leader(nullptr);
				} else {
					spdlog::error_s(
						"{} {} already leads {} {}!",
						get_branched_leader_name(new_leader->get_branch()), new_leader->get_name(),get_branched_unit_group_name(branch), name
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
	return !path.empty() ? path.back() : nullptr;
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
	std::string_view new_name
) : UnitInstanceGroup { new_unique_id, LAND, new_name } {}

void UnitInstanceGroupBranched<LAND>::update_gamestate() {
	UnitInstanceGroup::update_gamestate();
}

void UnitInstanceGroupBranched<LAND>::tick() {
	UnitInstanceGroup::tick();
}

UnitInstanceGroupBranched<NAVAL>::UnitInstanceGroupBranched(
	unique_id_t new_unique_id,
	std::string_view new_name
) : UnitInstanceGroup { new_unique_id, NAVAL, new_name } {}

void UnitInstanceGroupBranched<NAVAL>::update_gamestate() {
	UnitInstanceGroup::update_gamestate();
}

void UnitInstanceGroupBranched<NAVAL>::tick() {
	UnitInstanceGroup::tick();
}

fixed_point_t UnitInstanceGroupBranched<NAVAL>::get_total_consumed_supply() const {
	fixed_point_t total_consumed_supply = 0;

	for (ShipInstance const* ship : get_ship_instances()) {
		total_consumed_supply += ship->get_ship_type().get_supply_consumption_score();
	}

	return total_consumed_supply;
}

template<unit_branch_t Branch>
UnitInstanceBranched<Branch>& UnitInstanceManager::generate_unit_instance(UnitDeployment<Branch> const& unit_deployment) {
	UnitInstanceBranched<Branch>& unit_instance = *get_unit_instances<Branch>().insert(
		[this, &unit_deployment]() -> UnitInstanceBranched<Branch> {
			if constexpr (Branch == LAND) {
				return {
					unique_id_counter++,
					unit_deployment.get_name(),
					unit_deployment.get_type(),
					nullptr, // TODO - get pop from Province unit_deployment.get_home()
					false // Not mobilised
				};
			} else if constexpr (Branch == NAVAL) {
				return {
					unique_id_counter++,
					unit_deployment.get_name(),
					unit_deployment.get_type()
				};
			}
		}()
	);

	unit_instance_map.emplace(unit_instance.get_unique_id(), &unit_instance);

	return unit_instance;
}

template<unit_branch_t Branch>
bool UnitInstanceManager::generate_unit_instance_group(
	MapInstance& map_instance, CountryInstance& country, UnitDeploymentGroup<Branch> const& unit_deployment_group
) {
	if (unit_deployment_group.get_units().empty()) {
		spdlog::error_s(
			"Trying to generate unit group \"{}\" with no units for country \"{}\"",
			unit_deployment_group.get_name(), country
		);
		return false;
	}

	if (unit_deployment_group.get_location() == nullptr) {
		spdlog::error_s(
			"Trying to generate unit group \"{}\" with no location for country \"{}\"",
			unit_deployment_group.get_name(), country
		);
		return false;
	}

	UnitInstanceGroupBranched<Branch>& unit_instance_group = *get_unit_instance_groups<Branch>().insert({
		unique_id_counter++, unit_deployment_group.get_name()
	});
	unit_instance_group_map.emplace(unit_instance_group.get_unique_id(), &unit_instance_group);

	bool ret = true;

	for (UnitDeployment<Branch> const& unit_deployment : unit_deployment_group.get_units()) {
		ret &= unit_instance_group.add_unit(generate_unit_instance(unit_deployment));
	}

	ret &= unit_instance_group.set_position(
		&map_instance.get_province_instance_by_definition(*unit_deployment_group.get_location())
	);
	ret &= unit_instance_group.set_country(&country);

	if (unit_deployment_group.get_leader_index().has_value()) {
		memory::vector<LeaderInstance*>& leaders = country.get_leaders<Branch>();
		typename memory::vector<LeaderInstance*>::const_iterator it = leaders.begin();

		advance(it, *unit_deployment_group.get_leader_index());

		if (it < leaders.end()) {
			ret &= unit_instance_group.set_leader(*it);
		} else {
			spdlog::error_s(
				"Invalid leader index {} for unit group \"{}\" for country \"{}\"",
				*unit_deployment_group.get_leader_index(), unit_deployment_group.get_name(), country
			);
			ret = false;
		}
	}

	return ret;
}

void UnitInstanceManager::generate_leader(CountryInstance& country, LeaderBase const& leader) {
	LeaderInstance& leader_instance = *leaders.insert({
		unique_id_counter++, leader, country
	});
	leader_instance_map.emplace(leader_instance.get_unique_id(), &leader_instance);
	country.add_leader(leader_instance);

	if (leader_instance.get_picture().empty() && country.get_primary_culture() != nullptr) {
		leader_instance.set_picture(culture_manager.get_leader_picture_name(
			country.get_primary_culture()->get_group().get_leader(), leader.get_branch()
		));
	}
}

UnitInstanceManager::UnitInstanceManager(
	CultureManager const& new_culture_manager,
	LeaderTraitManager const& new_leader_trait_manager,
	MilitaryDefines const& new_military_defines
) : culture_manager { new_culture_manager },
	leader_trait_manager { new_leader_trait_manager },
	military_defines { new_military_defines } {}

bool UnitInstanceManager::generate_deployment(
	MapInstance& map_instance, CountryInstance& country, Deployment const* deployment
) {
	if (deployment == nullptr) {
		spdlog::error_s("Trying to generate null deployment for {}", country);
		return false;
	}

	bool ret = true;

	for (LeaderBase const& leader : deployment->get_leaders()) {
		generate_leader(country, leader);
	}

	const auto generate_group = [this, &map_instance, &country, &ret, deployment]<unit_branch_t Branch>() -> void {
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

LeaderInstance* UnitInstanceManager::get_leader_instance_by_unique_id(unique_id_t unique_id) {
	const decltype(leader_instance_map)::const_iterator it = leader_instance_map.find(unique_id);

	if (it != leader_instance_map.end()) {
		return it->second;
	} else {
		return nullptr;
	}
}

UnitInstance* UnitInstanceManager::get_unit_instance_by_unique_id(unique_id_t unique_id) {
	const decltype(unit_instance_map)::const_iterator it = unit_instance_map.find(unique_id);

	if (it != unit_instance_map.end()) {
		return it->second;
	} else {
		return nullptr;
	}
}

UnitInstanceGroup* UnitInstanceManager::get_unit_instance_group_by_unique_id(unique_id_t unique_id) {
	const decltype(unit_instance_group_map)::const_iterator it = unit_instance_group_map.find(unique_id);

	if (it != unit_instance_group_map.end()) {
		return it->second;
	} else {
		return nullptr;
	}
}

bool UnitInstanceManager::create_leader(
	CountryInstance& country,
	unit_branch_t branch,
	Date creation_date,
	std::string_view name,
	LeaderTrait const* personality,
	LeaderTrait const* background
) {
	const fixed_point_t leader_creation_cost = military_defines.get_leader_recruit_cost();
	if (country.get_leadership_point_stockpile() < leader_creation_cost) {
		spdlog::error_s(
			"Country \"{}\" does not have enough leadership points ({}) to create a {} (cost: {})",
			country,
			country.get_leadership_point_stockpile().to_string(2),
			get_branched_leader_name(branch),
			leader_creation_cost.to_string(2)
		);
		return false;
	}

	country.set_leadership_point_stockpile(country.get_leadership_point_stockpile() - leader_creation_cost);

	// TODO - replace with RNG
	static size_t item_selection_counter = 0;

	// Variable for storing a generated name if none is provided
	memory::string name_storage;
	if (name.empty()) {
		Culture const* culture = country.get_primary_culture();

		if (culture != nullptr) {
			std::string_view first_name, connector, last_name;

			if (!culture->get_first_names().empty()) {
				first_name = culture->get_first_names()[item_selection_counter++ % culture->get_first_names().size()];
			}

			if (!culture->get_last_names().empty()) {
				last_name = culture->get_last_names()[item_selection_counter++ % culture->get_last_names().size()];
			}

			if (!first_name.empty() && !last_name.empty()) {
				connector = " ";
			}

			name_storage = StringUtils::append_string_views(first_name, connector, last_name);
			name = name_storage;
		}
	}

	// TODO - use "noTrait" or "noPersonality" or "noBackground" if either is nullptr

	if (personality == nullptr && !leader_trait_manager.get_personality_traits().empty()) {
		personality = leader_trait_manager.get_personality_traits()[
			item_selection_counter++ % leader_trait_manager.get_personality_traits().size()
		];
	}

	if (background == nullptr && !leader_trait_manager.get_background_traits().empty()) {
		background = leader_trait_manager.get_background_traits()[
			item_selection_counter++ % leader_trait_manager.get_background_traits().size()
		];
	}

	// TODO - make starting prestige a random proportion of the maximum random prestige
	const fixed_point_t starting_prestige =
		military_defines.get_leader_max_random_prestige() * static_cast<int32_t>(item_selection_counter++ % 11) / 10;

	generate_leader(country, {
		name,
		branch,
		creation_date,
		personality,
		background,
		starting_prestige,
		{} // No picture, will be set up by generate_leader
	});

	return true;
}
