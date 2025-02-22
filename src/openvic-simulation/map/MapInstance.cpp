#include "MapInstance.hpp"

#include "openvic-simulation/history/ProvinceHistory.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/pop/PopValuesFromProvince.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

MapInstance::MapInstance(MapDefinition const& new_map_definition)
	: map_definition { new_map_definition }
	{}

ProvinceInstance& MapInstance::get_province_instance_from_definition(ProvinceDefinition const& province) {
	return province_instances.get_items()[province.get_index() - 1];
}

ProvinceInstance const& MapInstance::get_province_instance_from_definition(ProvinceDefinition const& province) const {
	return province_instances.get_items()[province.get_index() - 1];
}

bool MapInstance::setup(
	BuildingTypeManager const& building_type_manager,
	MarketInstance& market_instance,
	ModifierEffectCache const& modifier_effect_cache,
	PopsDefines const& pop_defines,
	decltype(ProvinceInstance::population_by_strata)::keys_type const& strata_keys,
	decltype(ProvinceInstance::pop_type_distribution)::keys_type const& pop_type_keys,
	decltype(ProvinceInstance::ideology_distribution)::keys_type const& ideology_keys
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

	if (ret) {
		const size_t max_worker_threads = std::thread::hardware_concurrency();
		const size_t max_threads_including_parent = max_worker_threads + 1;
		reusable_pop_values_collection.reserve(max_threads_including_parent);
		threads.reserve(max_worker_threads);

		for (size_t i = 0; i < max_worker_threads; i++) {
			reusable_pop_values_collection.emplace_back(PopValuesFromProvince {
				pop_defines, strata_keys
			});
			threads.emplace_back();
		}

		reusable_pop_values_collection.emplace_back(PopValuesFromProvince {
			pop_defines, strata_keys
		});
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
						std::optional<ProductionType const*> const& rgo_production_type_nullable_optional = entry->get_rgo_production_type_nullable();
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

void MapInstance::update_gamestate(const Date today, DefineManager const& define_manager) {
	highest_province_population = 0;
	total_map_population = 0;

	for (ProvinceInstance& province : province_instances.get_items()) {
		province.update_gamestate(today, define_manager);

		// Update population stats
		const pop_size_t province_population = province.get_total_population();
		if (highest_province_population < province_population) {
			highest_province_population = province_population;
		}

		total_map_population += province_population;
	}
	state_manager.update_gamestate();
}

void MapInstance::process_provinces_in_parallel(std::invocable<ProvinceInstance&, PopValuesFromProvince&> auto callback) {
	std::vector<ProvinceInstance>& provinces = province_instances.get_items();
	const auto [quotient, remainder] = std::ldiv(provinces.size(), reusable_pop_values_collection.size());
	std::vector<ProvinceInstance>::iterator begin = provinces.begin();
	for (size_t i = 0; i < threads.size(); i++) {
		const size_t chunk_size = i < remainder
			? quotient + 1
			: quotient;
			std::vector<ProvinceInstance>::iterator end = begin + chunk_size;
		threads[i] = std::thread{
			[
				&callback,
				&reusable_pop_values = reusable_pop_values_collection[i],
				begin,
				end
			]()->void{
				for (std::vector<ProvinceInstance>::iterator it = begin; it < end; it++) {
					callback(*it, reusable_pop_values);
				}
			}
		};
		begin = end;
	}
	{
		auto parent_thread_end = begin + quotient;
		auto& reusable_pop_values = reusable_pop_values_collection.back();
		for (auto it = begin; it < parent_thread_end; it++) {
			callback(*it, reusable_pop_values);
		}
	}
	for (std::thread& thread : threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}

void MapInstance::map_tick(const Date today) {
	process_provinces_in_parallel(
		[today](ProvinceInstance& province, PopValuesFromProvince& reusable_pop_values) -> void {
			province.province_tick(today, reusable_pop_values);
		}
	);
}

void MapInstance::initialise_for_new_game(
	const Date today,
	DefineManager const& define_manager
) {
	update_gamestate(today, define_manager);
	process_provinces_in_parallel(
		[today](ProvinceInstance& province, PopValuesFromProvince& reusable_pop_values) -> void {
			province.initialise_for_new_game(today, reusable_pop_values);
		}
	);
}