#include "MapInstance.hpp"
#include <optional>

#include "openvic-simulation/history/ProvinceHistory.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/pop/PopValuesFromProvince.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/ThreadPool.hpp"
#include "map/ProvinceDefinition.hpp"

using namespace OpenVic;

MapInstance::MapInstance(
	MapDefinition const& new_map_definition,
	ThreadPool& new_thread_pool
) : map_definition { new_map_definition },
	thread_pool { new_thread_pool },
	land_pathing { *this },
	sea_pathing { *this } {}

ProvinceInstance& MapInstance::get_province_instance_from_definition(ProvinceDefinition const& province) {
	return province_instances.get_items()[province.get_index()];
}
ProvinceInstance const& MapInstance::get_province_instance_from_definition(ProvinceDefinition const& province) const {
	return province_instances.get_items()[province.get_index()];
}

ProvinceInstance* MapInstance::get_province_instance_from_number(
	decltype(std::declval<ProvinceDefinition>().get_province_number())province_number
) {
	return province_instances.get_item_by_index(ProvinceDefinition::get_index_from_province_number(province_number));
}
ProvinceInstance const* MapInstance::get_province_instance_from_number(
	decltype(std::declval<ProvinceDefinition>().get_province_number())province_number
) const {
	return province_instances.get_item_by_index(ProvinceDefinition::get_index_from_province_number(province_number));
}

void MapInstance::enable_canal(canal_index_t canal_index) {
	enabled_canals.insert(canal_index);
}

bool MapInstance::is_canal_enabled(canal_index_t canal_index) const {
	return enabled_canals.contains(canal_index);
}

bool MapInstance::setup(
	BuildingTypeManager const& building_type_manager,
	MarketInstance& market_instance,
	GameRulesManager const& game_rules_manager,
	ModifierEffectCache const& modifier_effect_cache,
	decltype(ProvinceInstance::population_by_strata)::keys_span_type strata_keys,
	decltype(ProvinceInstance::pop_type_distribution)::keys_span_type pop_type_keys,
	decltype(ProvinceInstance::ideology_distribution)::keys_span_type ideology_keys
) {
	if (province_instances_are_locked()) {
		Logger::error("Cannot setup map instance - province instances are already locked!");
		return false;
	}

	bool ret = true;

	if (!map_definition.province_definitions_are_locked()) {
		Logger::error("Cannot setup map instance - province definitions are not locked!");
		ret = false;
	} else {
		province_instances.reserve(map_definition.get_province_definition_count());

		for (ProvinceDefinition const& province : map_definition.get_province_definitions()) {
			if (province_instances.add_item({
				market_instance,
				game_rules_manager,
				modifier_effect_cache,
				province,
				strata_keys,
				pop_type_keys,
				ideology_keys
			})) {
				// We need to update the province's ModifierSum's source here as the province's address is finally stable
				// after changing between its constructor call and now due to being std::move'd into the registry.
				ProvinceInstance& province_instance = get_back_province_instance();
				province_instance.modifier_sum.set_this_source(&province_instance);
			} else {
				ret = false;
			}
		}
	}

	province_instances.lock();

	for (ProvinceInstance& province : province_instances.get_items()) {
		ret &= province.setup(building_type_manager);
	}

	if (get_province_instance_count() != map_definition.get_province_definition_count()) {
		Logger::error(
			"ProvinceInstance count (", get_province_instance_count(), ") does not match ProvinceDefinition count (",
			map_definition.get_province_definition_count(), ")!"
		);
		return false;
	}

	return ret;
}

bool MapInstance::apply_history_to_provinces(
	ProvinceHistoryManager const& history_manager,
	const Date date,
	CountryInstanceManager& country_manager,
	IssueManager const& issue_manager,
	MarketInstance& market_instance,
	ArtisanalProducerFactoryPattern& artisanal_producer_factory_pattern
) {
	bool ret = true;

	for (ProvinceInstance& province : province_instances.get_items()) {
		ProvinceDefinition const& province_definition = province.get_province_definition();
		if (!province_definition.is_water()) {
			ProvinceHistoryMap const* history_map = history_manager.get_province_history(&province_definition);

			if (history_map != nullptr) {
				ProvinceHistoryEntry const* pop_history_entry = nullptr;
				ProductionType const* rgo_production_type_nullable = nullptr;

				for (auto const& [entry_date, entry] : history_map->get_entries()) {
					if (entry_date > date) {
						if (pop_history_entry != nullptr) {
							break;
						}
					} else {
						province.apply_history_to_province(*entry, country_manager);
						std::optional<ProductionType const*> const& rgo_production_type_nullable_optional =
							entry->get_rgo_production_type_nullable();
						if (rgo_production_type_nullable_optional.has_value()) {
							rgo_production_type_nullable = rgo_production_type_nullable_optional.value();
						}
					}

					if (!entry->get_pops().empty()) {
						pop_history_entry = entry.get();
					}
				}

				if (pop_history_entry == nullptr) {
					Logger::warning("No pop history entry for province ", province.get_identifier(), " for date ", date);
				} else {
					ret &= province.add_pop_vec(
						pop_history_entry->get_pops(),
						market_instance,
						artisanal_producer_factory_pattern
					);
					province.setup_pop_test_values(issue_manager);
				}

				ret &= province.set_rgo_production_type_nullable(rgo_production_type_nullable);
			}
		}
	}

	return ret;
}

void MapInstance::update_modifier_sums(const Date today, StaticModifierCache const& static_modifier_cache) {
	for (ProvinceInstance& province : province_instances.get_items()) {
		province.update_modifier_sum(today, static_modifier_cache);
	}
}

void MapInstance::update_gamestate(InstanceManager const& instance_manager) {
	highest_province_population = 0;
	total_map_population = 0;

	for (ProvinceInstance& province : province_instances.get_items()) {
		province.update_gamestate(instance_manager);

		// Update population stats
		const pop_size_t province_population = province.get_total_population();
		if (highest_province_population < province_population) {
			highest_province_population = province_population;
		}

		total_map_population += province_population;
	}
	state_manager.update_gamestate();
}

void MapInstance::map_tick() {
	thread_pool.process_province_ticks();
	//state tick
	//after province tick as province tick sets pop employment to 0
	//state tick will update pop employment via factories
}

void MapInstance::initialise_for_new_game(InstanceManager const& instance_manager) {
	update_gamestate(instance_manager);
	thread_pool.process_province_initialise_for_new_game();
}