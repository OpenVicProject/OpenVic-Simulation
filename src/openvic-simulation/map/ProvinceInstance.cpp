#include "ProvinceInstance.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/defines/Define.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/production/ResourceGatheringOperation.hpp"
#include "openvic-simulation/history/ProvinceHistory.hpp"
#include "openvic-simulation/map/Crime.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/Region.hpp"
#include "openvic-simulation/map/TerrainType.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/modifier/StaticModifierCache.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

ProvinceInstance::ProvinceInstance(
	MarketInstance& new_market_instance,
	ModifierEffectCache const& new_modifier_effect_cache,
	ProvinceDefinition const& new_province_definition,
	decltype(pop_type_distribution)::keys_t const& pop_type_keys,
	decltype(ideology_distribution)::keys_t const& ideology_keys
) : HasIdentifierAndColour { new_province_definition },
	province_definition { new_province_definition },
	terrain_type { new_province_definition.get_default_terrain_type() },
	life_rating { 0 },
	colony_status { colony_status_t::STATE },
	state { nullptr },
	owner { nullptr },
	controller { nullptr },
	cores {},
	modifier_sum {},
	event_modifiers {},
	slave { false },
	crime { nullptr },
	rgo { new_market_instance, new_modifier_effect_cache, pop_type_keys },
	buildings { "buildings", false },
	armies {},
	navies {},
	pops {},
	total_population { 0 },
	pop_type_distribution { &pop_type_keys },
	pops_cache_by_type { &pop_type_keys },
	ideology_distribution { &ideology_keys },
	culture_distribution {},
	religion_distribution {},
	max_supported_regiments { 0 }
	{}

GoodDefinition const* ProvinceInstance::get_rgo_good() const {
	if (!rgo.is_valid()) { return nullptr; }
	return &(rgo.get_production_type_nullable()->get_output_good());
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
		}
	}

	return ret;
}

bool ProvinceInstance::set_controller(CountryInstance* new_controller) {
	bool ret = true;

	if (controller != new_controller) {
		if (controller != nullptr) {
			ret &= controller->remove_controlled_province(*this);
		}

		controller = new_controller;

		if (controller != nullptr) {
			ret &= controller->add_controlled_province(*this);
		}
	}

	return ret;
}

bool ProvinceInstance::add_core(CountryInstance& new_core) {
	if (cores.emplace(&new_core).second) {
		return new_core.add_core_province(*this);
	} else {
		Logger::error(
			"Attempted to add core \"", new_core.get_identifier(), "\" to country ", get_identifier(), ": already exists!"
		);
		return false;
	}
}

bool ProvinceInstance::remove_core(CountryInstance& core_to_remove) {
	if (cores.erase(&core_to_remove) > 0) {
		return core_to_remove.remove_core_province(*this);
	} else {
		Logger::error(
			"Attempted to remove core \"", core_to_remove.get_identifier(), "\" from country ", get_identifier(),
			": does not exist!"
		);
		return false;
	}
}

bool ProvinceInstance::is_owner_core() const {
	return owner != nullptr && cores.contains(owner);
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

bool ProvinceInstance::add_pop_vec(std::vector<PopBase> const& pop_vec) {
	if (!province_definition.is_water()) {
		reserve_more(pops, pop_vec.size());
		for (PopBase const& pop : pop_vec) {
			_add_pop(Pop { pop, *ideology_distribution.get_keys() });
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
void ProvinceInstance::_update_pops(DefineManager const& define_manager) {
	total_population = 0;
	average_literacy = 0;
	average_consciousness = 0;
	average_militancy = 0;

	pop_type_distribution.clear();
	ideology_distribution.clear();
	culture_distribution.clear();
	religion_distribution.clear();

	for (PopType const& pop_type : *pops_cache_by_type.get_keys()) {
		pops_cache_by_type[pop_type].clear();
	}

	max_supported_regiments = 0;

	MilitaryDefines const& military_defines = define_manager.get_military_defines();

	using enum colony_status_t;

	const fixed_point_t pop_size_per_regiment_multiplier =
		colony_status == PROTECTORATE ? military_defines.get_pop_size_per_regiment_protectorate_multiplier()
		: colony_status == COLONY ? military_defines.get_pop_size_per_regiment_colony_multiplier()
		: is_owner_core() ? fixed_point_t::_1() : military_defines.get_pop_size_per_regiment_non_core_multiplier();

	for (Pop& pop : pops) {
		pop.update_gamestate(define_manager, owner, pop_size_per_regiment_multiplier);

		total_population += pop.get_size();
		average_literacy += pop.get_literacy();
		average_consciousness += pop.get_consciousness();
		average_militancy += pop.get_militancy();

		pop_type_distribution[*pop.get_type()] += pop.get_size();
		pops_cache_by_type[*pop.get_type()].push_back(&pop);
		ideology_distribution += pop.get_ideologies();
		culture_distribution[&pop.get_culture()] += pop.get_size();
		religion_distribution[&pop.get_religion()] += pop.get_size();

		max_supported_regiments += pop.get_max_supported_regiments();
	}

	if (total_population > 0) {
		average_literacy /= total_population;
		average_consciousness /= total_population;
		average_militancy /= total_population;
	}
}

void ProvinceInstance::update_modifier_sum(Date today, StaticModifierCache const& static_modifier_cache) {
	// Update sum of direct province modifiers
	modifier_sum.clear();

	const ModifierSum::modifier_source_t province_source { this };

	// Erase expired event modifiers and add non-expired ones to the sum
	std::erase_if(event_modifiers, [this, today, &province_source](ModifierInstance const& modifier) -> bool {
		if (today <= modifier.get_expiry_date()) {
			modifier_sum.add_modifier(*modifier.get_modifier(), province_source);
			return false;
		} else {
			return true;
		}
	});

	// Add static modifiers
	if (is_owner_core()) {
		modifier_sum.add_modifier(static_modifier_cache.get_core(), province_source);
	}
	if (province_definition.is_water()) {
		modifier_sum.add_modifier(static_modifier_cache.get_sea_zone(), province_source);
	} else {
		modifier_sum.add_modifier(static_modifier_cache.get_land_province(), province_source);

		if (province_definition.is_coastal()) {
			modifier_sum.add_modifier(static_modifier_cache.get_coastal(), province_source);
		} else {
			modifier_sum.add_modifier(static_modifier_cache.get_non_coastal(), province_source);
		}

		// TODO - overseas, blockaded, no_adjacent_controlled, has_siege, occupied, nationalism, infrastructure
	}

	for (BuildingInstance const& building : buildings.get_items()) {
		modifier_sum.add_modifier(building.get_building_type(), province_source);
	}

	modifier_sum.add_modifier_nullcheck(crime, province_source);

	modifier_sum.add_modifier_nullcheck(province_definition.get_continent(), province_source);

	modifier_sum.add_modifier_nullcheck(province_definition.get_climate(), province_source);

	modifier_sum.add_modifier_nullcheck(terrain_type, province_source);

	if constexpr (!ADD_OWNER_CONTRIBUTION) {
		if (controller != nullptr) {
			controller->contribute_province_modifier_sum(modifier_sum);
		}
	}
}

void ProvinceInstance::contribute_country_modifier_sum(ModifierSum const& owner_modifier_sum) {
	modifier_sum.add_modifier_sum_exclude_source(owner_modifier_sum, this);
}

fixed_point_t ProvinceInstance::get_modifier_effect_value(ModifierEffect const& effect) const {
	if constexpr (ADD_OWNER_CONTRIBUTION) {
		return modifier_sum.get_effect(effect);
	} else {
		using enum ModifierEffect::target_t;

		if (owner != nullptr) {
			if (ModifierEffect::excludes_targets(effect.get_targets(), PROVINCE)) {
				// Non-province targeted effects are already added to the country modifier sum
				return owner->get_modifier_effect_value(effect);
			} else {
				// Province-targeted effects aren't passed to the country modifier sum
				return owner->get_modifier_effect_value(effect) + modifier_sum.get_effect(effect);
			}
		} else {
			return modifier_sum.get_effect(effect);
		}
	}
}

fixed_point_t ProvinceInstance::get_modifier_effect_value_nullcheck(ModifierEffect const* effect) const {
	if (effect != nullptr) {
		return get_modifier_effect_value(*effect);
	} else {
		return fixed_point_t::_0();
	}
}

void ProvinceInstance::push_contributing_modifiers(
	ModifierEffect const& effect, std::vector<ModifierSum::modifier_entry_t>& contributions
) const {
	if constexpr (ADD_OWNER_CONTRIBUTION) {
		modifier_sum.push_contributing_modifiers(effect, contributions);
	} else {
		using enum ModifierEffect::target_t;

		if (owner != nullptr) {
			if (ModifierEffect::excludes_targets(effect.get_targets(), PROVINCE)) {
				// Non-province targeted effects are already added to the country modifier sum
				owner->push_contributing_modifiers(effect, contributions);
			} else {
				// Province-targeted effects aren't passed to the country modifier sum
				modifier_sum.push_contributing_modifiers(effect, contributions);
				owner->push_contributing_modifiers(effect, contributions);
			}
		} else {
			modifier_sum.push_contributing_modifiers(effect, contributions);
		}
	}
}

std::vector<ModifierSum::modifier_entry_t> ProvinceInstance::get_contributing_modifiers(ModifierEffect const& effect) const {
	if constexpr (ADD_OWNER_CONTRIBUTION) {
		return modifier_sum.get_contributing_modifiers(effect);
	} else {
		std::vector<ModifierSum::modifier_entry_t> contributions;

		push_contributing_modifiers(effect, contributions);

		return contributions;
	}
}

bool ProvinceInstance::convert_rgo_worker_pops_to_equivalent(ProductionType const& production_type) {
	bool is_valid_operation = true;
	std::vector<Job> const& jobs = production_type.get_jobs();
	for(Pop& pop : pops) {
		for(Job const& job : jobs) {
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

void ProvinceInstance::update_gamestate(const Date today, DefineManager const& define_manager) {
	for (BuildingInstance& building : buildings.get_items()) {
		building.update_gamestate(today);
	}
	_update_pops(define_manager);
}

void ProvinceInstance::province_tick(const Date today) {
	for (BuildingInstance& building : buildings.get_items()) {
		building.tick(today);
	}
	rgo.rgo_tick();
}

template<UnitType::branch_t Branch>
bool ProvinceInstance::add_unit_instance_group(UnitInstanceGroup<Branch>& group) {
	if (get_unit_instance_groups<Branch>().emplace(static_cast<UnitInstanceGroupBranched<Branch>*>(&group)).second) {
		return true;
	} else {
		Logger::error(
			"Trying to add already-existing ", Branch == UnitType::branch_t::LAND ? "army" : "navy", " ",
			group.get_name(), " to province ", get_identifier()
		);
		return false;
	}
}

template<UnitType::branch_t Branch>
bool ProvinceInstance::remove_unit_instance_group(UnitInstanceGroup<Branch>& group) {
	if (get_unit_instance_groups<Branch>().erase(static_cast<UnitInstanceGroupBranched<Branch>*>(&group)) > 0) {
		return true;
	} else {
		Logger::error(
			"Trying to remove non-existent ", Branch == UnitType::branch_t::LAND ? "army" : "navy", " ",
			group.get_name(), " from province ", get_identifier()
		);
		return false;
	}
}

template bool ProvinceInstance::add_unit_instance_group(UnitInstanceGroup<UnitType::branch_t::LAND>&);
template bool ProvinceInstance::add_unit_instance_group(UnitInstanceGroup<UnitType::branch_t::NAVAL>&);
template bool ProvinceInstance::remove_unit_instance_group(UnitInstanceGroup<UnitType::branch_t::LAND>&);
template bool ProvinceInstance::remove_unit_instance_group(UnitInstanceGroup<UnitType::branch_t::NAVAL>&);

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
			ret &= add_core(country_manager.get_country_instance_from_definition(*country));
		} else {
			ret &= remove_core(country_manager.get_country_instance_from_definition(*country));
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

void ProvinceInstance::initialise_rgo() {
	rgo.initialise_rgo_size_multiplier();
}

void ProvinceInstance::setup_pop_test_values(IssueManager const& issue_manager) {
	for (Pop& pop : pops) {
		pop.setup_pop_test_values(issue_manager);
	}
}

State* ProvinceInstance::get_mutable_state() {
	return state;
}

plf::colony<Pop>& ProvinceInstance::get_mutable_pops() {
	return pops;
}

IndexedMap<PopType, std::vector<Pop*>>& ProvinceInstance::get_mutable_pops_cache_by_type() {
	return pops_cache_by_type;
}