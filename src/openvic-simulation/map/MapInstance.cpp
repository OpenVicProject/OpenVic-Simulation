#include "MapInstance.hpp"

#include <functional>
#include <optional>
#include <tuple>

#include "openvic-simulation/history/ProvinceHistory.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/politics/Reform.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/population/PopValuesFromProvince.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/ThreadPool.hpp"

using namespace OpenVic;

MapInstance::MapInstance(
	MapDefinition const& new_map_definition,
	ProvinceInstanceDeps const& province_instance_deps,
	ThreadPool& new_thread_pool
) : map_definition { new_map_definition },
	thread_pool { new_thread_pool },
	land_pathing { new_map_definition, *this },
	sea_pathing { new_map_definition, *this },
	province_instance_by_definition(
		new_map_definition.get_province_definitions(),
		[&province_instance_deps](
			ProvinceDefinition const& province_definition
		) -> auto {
			return std::make_tuple(
				std::ref(province_definition),
				std::ref(province_instance_deps)
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

bool MapInstance::apply_history_to_province(
	ProvinceHistoryManager const& history_manager,
	const Date date,
	CountryInstanceManager& country_manager,
	MilitaryDefines const& military_defines,
	PopDeps const& pop_deps,
	TypedSpan<pop_type_index_t, const PopType> pop_types,
	TypedSpan<reform_index_t, const Reform> reforms,
	ProvinceInstance& province
) {
	ProvinceDefinition const& province_definition = province.province_definition;
	if (province_definition.is_water()) {
		return true;
	}

	ProvinceHistoryMap const* history_map = history_manager.get_province_history(&province_definition);

	if (history_map != nullptr) {
		return true;
	}

	bool ret = true;
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
			pop_deps
		);
		province.setup_pop_test_values(reforms);

		//update pops so OOB can use up to date max_supported_regiments
		province._update_pops(military_defines);
	}

	ret &= province.set_rgo_production_type_nullable(
		pop_types,
		rgo_production_type_nullable
	);

	return ret;
}

bool MapInstance::apply_history_to_provinces(
	ProvinceHistoryManager const& history_manager,
	const Date date,
	CountryInstanceManager& country_manager,
	MilitaryDefines const& military_defines,
	PopDeps const& pop_deps,
	TypedSpan<pop_type_index_t, const PopType> pop_types,
	TypedSpan<reform_index_t, const Reform> reforms
) {
	bool ret = true;

	for (ProvinceInstance& province : get_province_instances()) {
	}

	return ret;
}

void MapInstance::update_modifier_sums(const Date today) {
	thread_pool.process(work_t::PROVINCE_UPDATE_MODIFIER_SUMS);
}

void MapInstance::update_gamestate() {
	highest_province_population = 0;
	total_map_population = 0;

	thread_pool.process(work_t::PROVINCE_UPDATE_GAMESTATE);

	for (ProvinceInstance& province : get_province_instances()) {
		// Update population stats
		const pop_sum_t province_population = province.get_total_population();
		if (highest_province_population < province_population) {
			highest_province_population = province_population;
		}

		total_map_population += province_population;
	}
	state_manager.update_gamestate();
}

void MapInstance::map_tick() {
	thread_pool.process(work_t::PROVINCE_TICK);
	//state tick
	//after province tick as province tick sets pop employment to 0
	//state tick will update pop employment via factories
}

void MapInstance::initialise_for_new_game() {
	update_gamestate();
	thread_pool.process(work_t::PROVINCE_INITIALISE_FOR_NEW_GAME);
}