#include "MapInstance.hpp"

#include <optional>
#include <tuple>

#include "openvic-simulation/history/ProvinceHistory.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/pop/PopValuesFromProvince.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/ThreadPool.hpp"

#if defined(__GLIBCXX__) || defined(__GLIBCPP__) //libstdc++ requires a complete type when emplacing the province instance.
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#endif

using namespace OpenVic;

MapInstance::MapInstance(
	MapDefinition const& new_map_definition,
	BuildingTypeManager const& building_type_manager,
	GameRulesManager const& game_rules_manager,
	ModifierEffectCache const& modifier_effect_cache,
	MarketInstance& market_instance,
	ThreadPool& new_thread_pool,
	utility::forwardable_span<const Strata> strata_keys,
	utility::forwardable_span<const PopType> pop_type_keys,
	utility::forwardable_span<const Ideology> ideology_keys
) : map_definition { new_map_definition },
	thread_pool { new_thread_pool },
	land_pathing { *this },
	sea_pathing { *this },
	province_instance_by_definition(
		new_map_definition.get_province_definitions(),
		[
			market_instance_ptr=&market_instance,
			game_rules_manager_ptr=&game_rules_manager,
			modifier_effect_cache_ptr=&modifier_effect_cache,
			strata_keys,
			pop_type_keys,
			ideology_keys,
			building_type_manager_ptr=&building_type_manager
		](ProvinceDefinition const& province_definition) -> auto {
			return std::make_tuple(
				market_instance_ptr,
				game_rules_manager_ptr,
				modifier_effect_cache_ptr,
				&province_definition,
				strata_keys,
				pop_type_keys,
				ideology_keys,
				building_type_manager_ptr
			);
		}
	) { assert(new_map_definition.province_definitions_are_locked()); }

ProvinceInstance* MapInstance::get_province_instance_by_identifier(std::string_view identifier) {
	ProvinceDefinition const* province_definition = map_definition.get_province_definition_by_identifier(identifier);
	return province_definition == nullptr
		? nullptr
		: &get_province_instance_by_definition(*province_definition);
}
ProvinceInstance const* MapInstance::get_province_instance_by_identifier(std::string_view identifier) const {
	ProvinceDefinition const* province_definition = map_definition.get_province_definition_by_identifier(identifier);
	return province_definition == nullptr
		? nullptr
		: &get_province_instance_by_definition(*province_definition);
}
ProvinceInstance* MapInstance::get_province_instance_by_index(typename ProvinceInstance::index_t index) {
	return province_instance_by_definition.contains_index(index)
		? &province_instance_by_definition.at_index(index)
		: nullptr;
}
ProvinceInstance const* MapInstance::get_province_instance_by_index(typename ProvinceInstance::index_t index) const {
	return province_instance_by_definition.contains_index(index)
		? &province_instance_by_definition.at_index(index)
		: nullptr;
}
ProvinceInstance& MapInstance::get_province_instance_by_definition(ProvinceDefinition const& province_definition) {
	return province_instance_by_definition.at(province_definition);
}
ProvinceInstance const& MapInstance::get_province_instance_by_definition(ProvinceDefinition const& province_definition) const {
	return province_instance_by_definition.at(province_definition);
}

ProvinceInstance* MapInstance::get_province_instance_from_number(
	decltype(std::declval<ProvinceDefinition>().get_province_number())province_number
) {
	return get_province_instance_by_index(ProvinceDefinition::get_index_from_province_number(province_number));
}
ProvinceInstance const* MapInstance::get_province_instance_from_number(
	decltype(std::declval<ProvinceDefinition>().get_province_number())province_number
) const {
	return get_province_instance_by_index(ProvinceDefinition::get_index_from_province_number(province_number));
}

void MapInstance::enable_canal(canal_index_t canal_index) {
	enabled_canals.insert(canal_index);
}

bool MapInstance::is_canal_enabled(canal_index_t canal_index) const {
	return enabled_canals.contains(canal_index);
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

	for (ProvinceInstance& province : get_province_instances()) {
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
					spdlog::warn_s("No pop history entry for province {} for date {}", province, date);
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
	for (ProvinceInstance& province : get_province_instances()) {
		province.update_modifier_sum(today, static_modifier_cache);
	}
}

void MapInstance::update_gamestate(InstanceManager const& instance_manager) {
	highest_province_population = 0;
	total_map_population = 0;

	for (ProvinceInstance& province : get_province_instances()) {
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