#include "ProvinceInstance.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/defines/Define.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/production/ResourceGatheringOperation.hpp"
#include "openvic-simulation/history/ProvinceHistory.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/map/Crime.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/Region.hpp"
#include "openvic-simulation/map/TerrainType.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/modifier/StaticModifierCache.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/pop/PopValuesFromProvince.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

ProvinceInstance::ProvinceInstance(
	MarketInstance& new_market_instance,
	GameRulesManager const& new_game_rules_manager,
	ModifierEffectCache const& new_modifier_effect_cache,
	ProvinceDefinition const& new_province_definition,
	decltype(population_by_strata)::keys_type const& strata_keys,
	decltype(pop_type_distribution)::keys_type const& pop_type_keys,
	decltype(ideology_distribution)::keys_type const& ideology_keys
) : HasIdentifierAndColour { new_province_definition },
	HasIndex { new_province_definition.get_index() },
	FlagStrings { "province" },
	province_definition { new_province_definition },
	game_rules_manager { new_game_rules_manager },
	modifier_effect_cache { new_modifier_effect_cache },
	terrain_type { new_province_definition.get_default_terrain_type() },
	rgo { new_market_instance, pop_type_keys },
	buildings { "buildings", false },
	population_by_strata { &strata_keys },
	militancy_by_strata { &strata_keys },
	life_needs_fulfilled_by_strata { &strata_keys },
	everyday_needs_fulfilled_by_strata { &strata_keys },
	luxury_needs_fulfilled_by_strata { &strata_keys },
	pop_type_distribution { &pop_type_keys },
	pop_type_unemployed_count { &pop_type_keys },
	pops_cache_by_type { &pop_type_keys },
	ideology_distribution { &ideology_keys },
	vote_distribution { nullptr } {}

void ProvinceInstance::set_state(State* new_state) {
	// TODO - ensure this is removed from old state and added to new state (either here or wherever this is called from)
	// TODO - update pop factory employment
	state = new_state;
}

bool ProvinceInstance::set_rgo_production_type_nullable(ProductionType const* rgo_production_type_nullable) {
	bool is_valid_operation = true;
	if (rgo_production_type_nullable != nullptr) {
		ProductionType const& rgo_production_type = *rgo_production_type_nullable;
		if (rgo_production_type.get_template_type() != ProductionType::template_type_t::RGO) {
			Logger::error("Tried setting province ", get_identifier(), " rgo to ", rgo_production_type.get_identifier(), " which is not of template_type RGO.");
			is_valid_operation = false;
		}
		is_valid_operation&=convert_rgo_worker_pops_to_equivalent(rgo_production_type);
	}

	rgo.set_production_type_nullable(rgo_production_type_nullable);
	return is_valid_operation;
}

bool ProvinceInstance::set_owner(CountryInstance* new_owner) {
	bool ret = true;

	if (owner != new_owner) {
		if (owner != nullptr) {
			ret &= owner->remove_owned_province(*this);
		}

		owner = new_owner;

		if (owner != nullptr) {
			ret &= owner->add_owned_province(*this);

			vote_distribution.set_keys(&owner->get_country_definition()->get_parties());
		} else {
			vote_distribution.set_keys(nullptr);
		}

		for (Pop& pop : pops) {
			pop.update_location_based_attributes();
		}

		if (game_rules_manager.get_country_to_report_economy() == country_to_report_economy_t::Owner) {
			country_to_report_economy = new_owner;
		} else if (game_rules_manager.get_country_to_report_economy() == country_to_report_economy_t::NeitherWhenOccupied) {
			country_to_report_economy = is_occupied()
				? nullptr
				: owner;
		}
	}

	return ret;
}

bool ProvinceInstance::set_controller(CountryInstance* new_controller) {
	bool ret = true;

	if (new_controller == nullptr) {
		new_controller = owner;
	}

	if (controller != new_controller) {
		if (controller != nullptr) {
			ret &= controller->remove_controlled_province(*this);
		}

		controller = new_controller;

		if (controller != nullptr) {
			ret &= controller->add_controlled_province(*this);
		}

		if (game_rules_manager.get_country_to_report_economy() == country_to_report_economy_t::Controller) {
			country_to_report_economy = new_controller;
		} else if (game_rules_manager.get_country_to_report_economy() == country_to_report_economy_t::NeitherWhenOccupied) {
			country_to_report_economy = is_occupied()
				? nullptr
				: owner;
		}
	}

	return ret;
}

bool ProvinceInstance::add_core(CountryInstance& new_core, bool warn) {
	if (cores.emplace(&new_core).second) {
		return new_core.add_core_province(*this);
	} else if (warn) {
		Logger::warning(
			"Attempted to add core \"", new_core.get_identifier(), "\" to province ", get_identifier(), ": already exists!"
		);
	}
	return true;
}

bool ProvinceInstance::remove_core(CountryInstance& core_to_remove, bool warn) {
	if (cores.erase(&core_to_remove) > 0) {
		return core_to_remove.remove_core_province(*this);
	} else if (warn) {
		Logger::warning(
			"Attempted to remove core \"", core_to_remove.get_identifier(), "\" from province ", get_identifier(),
			": does not exist!"
		);
	}
	return true;
}

fixed_point_t ProvinceInstance::get_issue_support(Issue const& issue) const {
	const decltype(issue_distribution)::const_iterator it = issue_distribution.find(&issue);

	if (it != issue_distribution.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

fixed_point_t ProvinceInstance::get_party_support(CountryParty const& party) const {
	if (vote_distribution.has_keys()) {
		return vote_distribution[party];
	} else {
		return fixed_point_t::_0();
	}
}

fixed_point_t ProvinceInstance::get_culture_proportion(Culture const& culture) const {
	const decltype(culture_distribution)::const_iterator it = culture_distribution.find(&culture);

	if (it != culture_distribution.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

fixed_point_t ProvinceInstance::get_religion_proportion(Religion const& religion) const {
	const decltype(religion_distribution)::const_iterator it = religion_distribution.find(&religion);

	if (it != religion_distribution.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

bool ProvinceInstance::expand_building(size_t building_index) {
	BuildingInstance* building = buildings.get_item_by_index(building_index);
	if (building == nullptr) {
		Logger::error("Trying to expand non-existent building index ", building_index, " in province ", get_identifier());
		return false;
	}
	return building->expand();
}

void ProvinceInstance::_add_pop(Pop&& pop) {
	pop.set_location(*this);
	pops.insert(std::move(pop));
}

bool ProvinceInstance::add_pop(Pop&& pop) {
	if (!province_definition.is_water()) {
		_add_pop(std::move(pop));
		return true;
	} else {
		Logger::error("Trying to add pop to water province ", get_identifier());
		return false;
	}
}

bool ProvinceInstance::add_pop_vec(
	std::vector<PopBase> const& pop_vec,
	MarketInstance& market_instance,
	ArtisanalProducerFactoryPattern& artisanal_producer_factory_pattern
) {
	if (!province_definition.is_water()) {
		reserve_more(pops, pop_vec.size());
		for (PopBase const& pop : pop_vec) {
			_add_pop(Pop {
				pop,
				*ideology_distribution.get_keys(),
				market_instance,
				artisanal_producer_factory_pattern
			});
		}
		return true;
	} else {
		Logger::error("Trying to add pop vector to water province ", get_identifier());
		return false;
	}
}

size_t ProvinceInstance::get_pop_count() const {
	return pops.size();
}

/* REQUIREMENTS:
 * MAP-65, MAP-68, MAP-70, MAP-234
 */
void ProvinceInstance::_update_pops(InstanceManager const& instance_manager) {
	total_population = 0;
	average_literacy = 0;
	average_consciousness = 0;
	average_militancy = 0;

	population_by_strata.clear();
	militancy_by_strata.clear();
	life_needs_fulfilled_by_strata.clear();
	everyday_needs_fulfilled_by_strata.clear();
	luxury_needs_fulfilled_by_strata.clear();

	pop_type_distribution.clear();
	pop_type_unemployed_count.clear();
	ideology_distribution.clear();
	issue_distribution.clear();
	vote_distribution.clear();
	culture_distribution.clear();
	religion_distribution.clear();

	has_unaccepted_pops = false;

	for (std::vector<Pop*>& pops_cache : pops_cache_by_type.get_values()) {
		pops_cache.clear();
	}

	max_supported_regiments = 0;

	MilitaryDefines const& military_defines =
		instance_manager.get_definition_manager().get_define_manager().get_military_defines();

	using enum colony_status_t;

	const fixed_point_t pop_size_per_regiment_multiplier =
		colony_status == PROTECTORATE ? military_defines.get_pop_size_per_regiment_protectorate_multiplier()
		: colony_status == COLONY ? military_defines.get_pop_size_per_regiment_colony_multiplier()
		: is_owner_core() ? fixed_point_t::_1() : military_defines.get_pop_size_per_regiment_non_core_multiplier();

	for (Pop& pop : pops) {
		pop.update_gamestate(instance_manager, owner, pop_size_per_regiment_multiplier);

		const pop_size_t pop_size_s = pop.get_size();
		// TODO - change casting if pop_size_t changes type
		const fixed_point_t pop_size_f = fixed_point_t::parse(pop_size_s);

		total_population += pop_size_s;
		average_literacy += pop.get_literacy() * pop_size_f;
		average_consciousness += pop.get_consciousness() * pop_size_f;
		average_militancy += pop.get_militancy() * pop_size_f;

		PopType const& pop_type = *pop.get_type();
		Strata const& strata = pop_type.get_strata();

		population_by_strata[strata] += pop_size_s;
		militancy_by_strata[strata] += pop.get_militancy() * pop_size_f;
		life_needs_fulfilled_by_strata[strata] += pop.get_life_needs_fulfilled() * pop_size_f;
		everyday_needs_fulfilled_by_strata[strata] += pop.get_everyday_needs_fulfilled() * pop_size_f;
		luxury_needs_fulfilled_by_strata[strata] += pop.get_luxury_needs_fulfilled() * pop_size_f;

		pop_type_distribution[pop_type] += pop_size_s;
		pop_type_unemployed_count[pop_type] += pop.get_unemployed();
		pops_cache_by_type[pop_type].push_back(&pop);
		// Pop ideology, issue and vote distributions are scaled to pop size so we can add them directly
		ideology_distribution += pop.get_ideology_distribution();
		issue_distribution += pop.get_issue_distribution();
		vote_distribution += pop.get_vote_distribution();
		culture_distribution[&pop.get_culture()] += pop_size_f;
		religion_distribution[&pop.get_religion()] += pop_size_f;

		max_supported_regiments += pop.get_max_supported_regiments();

		if (pop.get_culture_status() == Pop::culture_status_t::UNACCEPTED) {
			has_unaccepted_pops = true;
		}
	}

	if (total_population > 0) {
		average_literacy /= total_population;
		average_consciousness /= total_population;
		average_militancy /= total_population;

		militancy_by_strata /= population_by_strata;
		life_needs_fulfilled_by_strata /= population_by_strata;
		everyday_needs_fulfilled_by_strata /= population_by_strata;
		luxury_needs_fulfilled_by_strata /= population_by_strata;

		unemployment_fraction = pop_type_unemployed_count.get_total() / total_population;

		const fixed_point_map_const_iterator_t<Culture const*> largest_culture_it = get_largest_item(culture_distribution);
		largest_culture = largest_culture_it != culture_distribution.end() ? largest_culture_it->first : nullptr;

		const fixed_point_map_const_iterator_t<Religion const*> largest_religion_it = get_largest_item(religion_distribution);
		largest_religion = largest_religion_it != religion_distribution.end() ? largest_religion_it->first : nullptr;
	} else {
		unemployment_fraction = fixed_point_t::_0();

		largest_culture = nullptr;
		largest_religion = nullptr;
	}
}

void ProvinceInstance::update_modifier_sum(Date today, StaticModifierCache const& static_modifier_cache) {
	// Update sum of direct province modifiers
	modifier_sum.clear();

	if (terrain_type != nullptr) {
		modifier_sum.add_modifier(*terrain_type);
	}

	if (province_definition.get_climate() != nullptr) {
		modifier_sum.add_modifier(*province_definition.get_climate());
	}

	if (province_definition.get_continent() != nullptr) {
		modifier_sum.add_modifier(*province_definition.get_continent());
	}

	// Add static modifiers
	if (is_owner_core()) {
		modifier_sum.add_modifier(static_modifier_cache.get_core());
	}
	if (province_definition.is_water()) {
		modifier_sum.add_modifier(static_modifier_cache.get_sea_zone());
	} else {
		modifier_sum.add_modifier(static_modifier_cache.get_land_province());

		modifier_sum.add_modifier(
			province_definition.is_coastal() ? static_modifier_cache.get_coastal() : static_modifier_cache.get_non_coastal()
		);

		// TODO - overseas, blockaded, no_adjacent_controlled, has_siege, occupied, nationalism, infrastructure
	}

	// Erase expired event modifiers and add non-expired ones to the sum
	std::erase_if(event_modifiers, [this, today](ModifierInstance const& modifier) -> bool {
		if (today <= modifier.get_expiry_date()) {
			modifier_sum.add_modifier(*modifier.get_modifier());
			return false;
		} else {
			return true;
		}
	});

	for (BuildingInstance const& building : buildings.get_items()) {
		modifier_sum.add_modifier(building.get_building_type());
	}

	if (crime != nullptr) {
		modifier_sum.add_modifier(*crime);
	}

	if (controller != nullptr) {
		controller->contribute_province_modifier_sum(modifier_sum);
	}
}

fixed_point_t ProvinceInstance::get_modifier_effect_value(ModifierEffect const& effect) const {
	using enum ModifierEffect::target_t;

	if (effect.is_local()) {
		// Province-targeted/local effects come from the province itself, only modifiers applied directly to the
		// province contribute to these effects.
		return modifier_sum.get_modifier_effect_value(effect);
	} else if (owner != nullptr) {
		// Non-province targeted/global effects come from the province's owner, even those applied locally
		// (e.g. via a province event modifier) are passed up to the province's controller and only affect the
		// province if the controller is also the owner.
		return owner->get_modifier_effect_value(effect);
	} else {
		return fixed_point_t::_0();
	}
}

bool ProvinceInstance::convert_rgo_worker_pops_to_equivalent(ProductionType const& production_type) {
	bool is_valid_operation = true;
	std::vector<Job> const& jobs = production_type.get_jobs();
	for (Pop& pop : pops) {
		for (Job const& job : jobs) {
			PopType const* const job_pop_type = job.get_pop_type();
			PopType const* old_pop_type = pop.get_type();
			if (job_pop_type != old_pop_type) {
				PopType const* const equivalent = old_pop_type->get_equivalent();
				if (job_pop_type == equivalent) {
					is_valid_operation&=pop.convert_to_equivalent();
				}
			}
		}
	}
	return is_valid_operation;
}

void ProvinceInstance::update_gamestate(InstanceManager const& instance_manager) {
	has_empty_adjacent_province = false;
	// We assume there are no duplicate province adjacencies, so each adjacency.get_to() is unique in the loop below
	adjacent_nonempty_land_provinces.clear();

	MapInstance const& map_instance = instance_manager.get_map_instance();
	for (ProvinceDefinition::adjacency_t const& adjacency : province_definition.get_adjacencies()) {
		ProvinceDefinition const& province_definition = *adjacency.get_to();
		ProvinceInstance const& province_instance = map_instance.get_province_instance_from_definition(province_definition);

		if (province_instance.is_empty()) {
			has_empty_adjacent_province = true;
		} else if (!province_definition.is_water()) {
			adjacent_nonempty_land_provinces.push_back(&province_instance);
		}
	}

	land_regiment_count = 0;
	for (ArmyInstance const* army : armies) {
		land_regiment_count += army->get_unit_count();
	}
	for (NavyInstance const* navy : navies) {
		for (ArmyInstance const* army : navy->get_carried_armies()) {
			land_regiment_count += army->get_unit_count();
		}
	}

	if (!is_occupied()) {
		occupation_duration = 0;
	}

	const Date today = instance_manager.get_today();

	for (BuildingInstance& building : buildings.get_items()) {
		building.update_gamestate(today);
	}
	_update_pops(instance_manager);
}

void ProvinceInstance::province_tick(
	const Date today,
	PopValuesFromProvince& reusable_pop_values,
	std::vector<fixed_point_t>& reusable_vector
) {
	if (is_occupied()) {
		occupation_duration++;
	}

	reusable_pop_values.update_pop_values_from_province(*this);
	for (Pop& pop : pops) {
		pop.pop_tick(reusable_pop_values, reusable_vector);
	}
	for (BuildingInstance& building : buildings.get_items()) {
		building.tick(today);
	}
	rgo.rgo_tick(reusable_vector);
}

bool ProvinceInstance::add_unit_instance_group(UnitInstanceGroup& group) {
	using enum UnitType::branch_t;

	switch (group.get_branch()) {
	case LAND:
		armies.push_back(static_cast<ArmyInstance*>(&group));
		return true;
	case NAVAL:
		navies.push_back(static_cast<NavyInstance*>(&group));
		return true;
	default:
		Logger::error(
			"Trying to add unit group \"", group.get_name(), "\" with invalid branch ",
			static_cast<uint32_t>(group.get_branch()), " to province ", get_identifier()
		);
		return false;
	}
}

bool ProvinceInstance::remove_unit_instance_group(UnitInstanceGroup const& group) {
	const auto remove_from_vector = [this, &group]<UnitType::branch_t Branch>(
		std::vector<UnitInstanceGroupBranched<Branch>*>& unit_instance_groups
	) -> bool {
		const typename std::vector<UnitInstanceGroupBranched<Branch>*>::const_iterator it =
			std::find(unit_instance_groups.begin(), unit_instance_groups.end(), &group);

		if (it != unit_instance_groups.end()) {
			unit_instance_groups.erase(it);
			return true;
		} else {
			Logger::error(
				"Trying to remove non-existent ", UnitType::get_branched_unit_group_name(Branch), " \"",
				group.get_name(), "\" from province ", get_identifier()
			);
			return false;
		}
	};

	using enum UnitType::branch_t;

	switch (group.get_branch()) {
	case LAND:
		return remove_from_vector(armies);
	case NAVAL:
		return remove_from_vector(navies);
	default:
		Logger::error(
			"Trying to remove unit group \"", group.get_name(), "\" with invalid branch ",
			static_cast<uint32_t>(group.get_branch()), " from province ", get_identifier()
		);
		return false;
	}
}

bool ProvinceInstance::setup(BuildingTypeManager const& building_type_manager) {
	if (buildings_are_locked()) {
		Logger::error("Cannot setup province ", get_identifier(), " - buildings already locked!");
		return false;
	}

	rgo.setup_location_ptr(*this);

	bool ret = true;

	if (!province_definition.is_water()) {
		if (building_type_manager.building_types_are_locked()) {
			buildings.reserve(building_type_manager.get_province_building_types().size());

			for (BuildingType const* building_type : building_type_manager.get_province_building_types()) {
				ret &= buildings.add_item({ *building_type });
			}
		} else {
			Logger::error("Cannot generate buildings until building types are locked!");
			ret = false;
		}
	}

	lock_buildings();

	return ret;
}

bool ProvinceInstance::apply_history_to_province(ProvinceHistoryEntry const& entry, CountryInstanceManager& country_manager) {
	bool ret = true;

	constexpr auto set_optional = []<typename T>(T& target, std::optional<T> const& source) {
		if (source) {
			target = *source;
		}
	};

	if (entry.get_owner()) {
		ret &= set_owner(&country_manager.get_country_instance_from_definition(**entry.get_owner()));
	}
	if (entry.get_controller()) {
		ret &= set_controller(&country_manager.get_country_instance_from_definition(**entry.get_controller()));
	}
	set_optional(colony_status, entry.get_colonial());
	set_optional(slave, entry.get_slave());
	for (auto const& [country, add] : entry.get_cores()) {
		if (add) {
			ret &= add_core(country_manager.get_country_instance_from_definition(*country), true);
		} else {
			ret &= remove_core(country_manager.get_country_instance_from_definition(*country), true);
		}
	}

	set_optional(life_rating, entry.get_life_rating());
	set_optional(terrain_type, entry.get_terrain_type());
	for (auto const& [building, level] : entry.get_province_buildings()) {
		BuildingInstance* existing_entry = buildings.get_item_by_identifier(building->get_identifier());
		if (existing_entry != nullptr) {
			existing_entry->set_level(level);
		} else {
			Logger::error(
				"Trying to set level of non-existent province building ", building->get_identifier(), " to ", level,
				" in province ", get_identifier()
			);
			ret = false;
		}
	}
	// TODO: load state buildings - entry.get_state_buildings()
	// TODO: party loyalties for each POP when implemented on POP side - entry.get_party_loyalties()
	return ret;
}

void ProvinceInstance::initialise_for_new_game(
	const Date today,
	PopValuesFromProvince& reusable_pop_values,
	std::vector<fixed_point_t>& reusable_vector
) {
	initialise_rgo();
	province_tick(today, reusable_pop_values, reusable_vector);
}

void ProvinceInstance::initialise_rgo() {
	rgo.initialise_rgo_size_multiplier();
}

void ProvinceInstance::setup_pop_test_values(IssueManager const& issue_manager) {
	for (Pop& pop : pops) {
		pop.setup_pop_test_values(issue_manager);
	}
}

plf::colony<Pop>& ProvinceInstance::get_mutable_pops() {
	return pops;
}