#include "UnitInstanceManager.hpp"

#include <fmt/std.h>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/defines/MilitaryDefines.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/military/Deployment.hpp"
#include "openvic-simulation/military/LeaderTraitManager.hpp"
#include "openvic-simulation/population/CultureManager.hpp"
#include "openvic-simulation/population/PopType.hpp"

using namespace OpenVic;

using enum unit_branch_t;

Pop* UnitInstanceManager::recruit_pop_in(ProvinceInstance& province, const bool is_rebel) const {
	if (is_rebel) {
		for (auto& pop : province.get_mutable_pops()) {
			if (pop.get_rebel_type() != nullptr && pop.try_recruit()) {
				return &pop;
			}
		}
	} else {
		for (auto& pop : province.get_mutable_pops()) {
			if (pop.get_type().can_be_recruited && pop.try_recruit()) {
				/*
				Victoria 2 does not respect cultural restrictions when applying history.
				*/
				return &pop;
			}
		}
	}

	//fallback to understrength pops
	if (is_rebel) {
		for (auto& pop : province.get_mutable_pops()) {
			if (pop.get_rebel_type() != nullptr && pop.try_recruit_understrength()) {
				return &pop;
			}
		}
	} else {
		for (auto& pop : province.get_mutable_pops()) {
			if (pop.get_type().can_be_recruited && pop.try_recruit_understrength()) {
				/*
				Victoria 2 does not respect cultural restrictions when applying history.
				*/
				return &pop;
			}
		}
	}

	return nullptr;
}

template<unit_branch_t Branch>
UnitInstanceBranched<Branch>& UnitInstanceManager::generate_unit_instance(
	UnitDeployment<Branch> const& unit_deployment,
	MapInstance& map_instance,
	const bool is_rebel
) {
	UnitInstanceBranched<Branch>& unit_instance = *get_unit_instances<Branch>().insert(
		[this, &unit_deployment, &map_instance, is_rebel]() -> UnitInstanceBranched<Branch> {
			if constexpr (Branch == LAND) {
				RegimentDeployment const& regiment_deployment = unit_deployment;
				ProvinceInstance& province = map_instance.get_province_instance_by_definition(*regiment_deployment.get_home());
				Pop* const pop_ptr = recruit_pop_in(province, is_rebel);
				if (pop_ptr == nullptr) {		
					spdlog::warn_s(
						"Regiment {} in province {} lacks backing pop.", regiment_deployment.get_name(), province.get_identifier()
					);
				}

				return {
					unique_id_counter++,
					unit_deployment.get_name(),
					unit_deployment.type,
					pop_ptr,
					false
				};
			} else if constexpr (Branch == NAVAL) {
				return {
					unique_id_counter++,
					unit_deployment.get_name(),
					unit_deployment.type
				};
			}
		}()
	);

	unit_instance_map.emplace(unit_instance.unique_id, unit_instance);

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
	
	ProvinceInstance& location = map_instance.get_province_instance_by_definition(unit_deployment_group.get_location());
	UnitInstanceGroupBranched<Branch>& unit_instance_group = *get_unit_instance_groups<Branch>().emplace(
		unique_id_counter++,
		unit_deployment_group.get_name(),
		country,
		location
	);
	unit_instance_group_map.emplace(unit_instance_group.unique_id, unit_instance_group);

	bool ret = true;

	for (UnitDeployment<Branch> const& unit_deployment : unit_deployment_group.get_units()) {
		ret &= unit_instance_group.add_unit(
			generate_unit_instance(unit_deployment, map_instance, country.is_rebel_country())
		);
	}

	ret &= unit_instance_group.set_country(country);

	if (unit_deployment_group.get_leader_index().has_value()) {
		memory::vector<std::reference_wrapper<LeaderInstance>>& leaders = country.get_leaders<Branch>();
		auto it = leaders.begin();

		advance(it, *unit_deployment_group.get_leader_index());

		if (it < leaders.end()) {
			ret &= unit_instance_group.set_leader(&it->get());
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

template<typename T>
void UnitInstanceManager::generate_leader(CountryInstance& country, T&& leader_base) {
	LeaderInstance& leader_instance = *leaders.emplace(
		unique_id_counter++,
		std::forward<T>(leader_base),
		country
	);
	leader_instance_map.emplace(leader_instance.unique_id, leader_instance);
	country.add_leader(leader_instance);

	if (leader_instance.get_picture().empty() && country.get_primary_culture() != nullptr) {
		leader_instance.set_picture(culture_manager.get_leader_picture_name(
			country.get_primary_culture()->group.get_leader(), leader_instance.branch
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
	MapInstance& map_instance, CountryInstance& country, Deployment const& deployment
) {
	bool ret = true;

	for (LeaderBase const& leader : deployment.get_leaders()) {
		generate_leader(country, leader);
	}

	const auto generate_group = [this, &map_instance, &country, &ret, &deployment]<unit_branch_t Branch>() -> void {
		for (UnitDeploymentGroup<Branch> const& unit_deployment_group : deployment.get_unit_deployment_groups<Branch>()) {
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
		return &it->second.get();
	} else {
		return nullptr;
	}
}

UnitInstance* UnitInstanceManager::get_unit_instance_by_unique_id(unique_id_t unique_id) {
	const decltype(unit_instance_map)::const_iterator it = unit_instance_map.find(unique_id);

	if (it != unit_instance_map.end()) {
		return &it->second.get();
	} else {
		return nullptr;
	}
}

UnitInstanceGroup* UnitInstanceManager::get_unit_instance_group_by_unique_id(unique_id_t unique_id) {
	const decltype(unit_instance_group_map)::const_iterator it = unit_instance_group_map.find(unique_id);

	if (it != unit_instance_group_map.end()) {
		return &it->second.get();
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
			"Country \"{}\" does not have enough leadership points ({:.2}) to create a {} (cost: {:.2})",
			country,
			country.get_leadership_point_stockpile(),
			get_branched_leader_name(branch),
			leader_creation_cost
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

			name_storage = append_string_views(first_name, connector, last_name);
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

	generate_leader(country, LeaderBase{
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