#include "ProvinceHistory.hpp"

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

ProvinceHistoryEntry::ProvinceHistoryEntry(ProvinceDefinition const& new_province, Date new_date)
	: HistoryEntry { new_date }, province { new_province } {}

ProvinceHistoryMap::ProvinceHistoryMap(ProvinceDefinition const& new_province) : province { new_province } {}

memory::unique_ptr<ProvinceHistoryEntry> ProvinceHistoryMap::_make_entry(Date date) const {
	return memory::make_unique<ProvinceHistoryEntry>(province, date);
}

bool ProvinceHistoryMap::_load_history_entry(
	DefinitionManager const& definition_manager, ProvinceHistoryEntry& entry, ast::NodeCPtr root
) {
	BuildingTypeManager const& building_type_manager = definition_manager.get_economy_manager().get_building_type_manager();
	CountryDefinitionManager const& country_definition_manager = definition_manager.get_country_definition_manager();
	GoodDefinitionManager const& good_definition_manager =
		definition_manager.get_economy_manager().get_good_definition_manager();
	IdeologyManager const& ideology_manager = definition_manager.get_politics_manager().get_ideology_manager();
	TerrainTypeManager const& terrain_type_manager = definition_manager.get_map_definition().get_terrain_type_manager();

	using enum ProvinceInstance::colony_status_t;
	static const string_map_t<ProvinceInstance::colony_status_t> colony_status_map {
		{ "0", STATE }, { "1", PROTECTORATE }, { "2", COLONY }
	};

	const auto set_core_instruction = [&entry](bool add) {
		return [&entry, add](CountryDefinition const& country) -> bool {
			const auto it = entry.cores.find(&country);
			if (it == entry.cores.end()) {
				// No current core instruction
				entry.cores.emplace(&country, add);
				return true;
			} else if (it->second == add) {
				// Desired core instruction already exists
				Logger::warning(
					"Duplicate attempt to ", add ? "add" : "remove", " core of country ", country.get_identifier(),
					" ", add ? "to" : "from", " province history of ", entry.get_province()
				);
				return true;
			} else {
				// Opposite core instruction exists
				entry.cores.erase(it);
				Logger::warning(
					"Attempted to ", add ? "add" : "remove", " core of country ", country.get_identifier(),
					" ", add ? "to" : "from", " province history of ", entry.get_province(),
					" after previously ", add ? "removing" : "adding", " it"
				);
				return true;
			}
		};
	};

	constexpr bool allow_empty_true = true;
	constexpr bool do_warn = true;

	return expect_dictionary_keys_and_default(
		[this, &definition_manager, &building_type_manager, &entry](
			std::string_view key, ast::NodeCPtr value) -> bool {
			// used for province buildings like forts or railroads
			BuildingType const* building_type = building_type_manager.get_building_type_by_identifier(key);
			if (building_type != nullptr) {
				if (building_type->is_in_province()) {
					return expect_uint<BuildingType::level_t>(
						/* This is set to warn to prevent vanilla from always having errors because
						 * of a duplicate railroad entry in the 1861.1.1 history of Manchester (278). */
						map_callback(entry.province_buildings, building_type, true)
					)(value);
				} else {
					Logger::error(
						"Attempted to add state building \"", building_type, "\" at top scope of province history for ",
						entry.get_province()
					);
					return false;
				}
			}

			return _load_history_sub_entry_callback(definition_manager, entry.get_date(), value, key, value);
		},
		"owner", ZERO_OR_ONE, country_definition_manager.expect_country_definition_identifier(
			assign_variable_callback_pointer_opt(entry.owner, true)
		),
		"controller", ZERO_OR_ONE, country_definition_manager.expect_country_definition_identifier(
			assign_variable_callback_pointer_opt(entry.controller, true)
		),
		"add_core", ZERO_OR_MORE, country_definition_manager.expect_country_definition_identifier(
			set_core_instruction(true)
		),
		"remove_core", ZERO_OR_MORE, country_definition_manager.expect_country_definition_identifier(
			set_core_instruction(false)
		),
		"colonial", ZERO_OR_ONE,
			expect_identifier(expect_mapped_string(colony_status_map, assign_variable_callback(entry.colonial))),
		"colony", ZERO_OR_ONE,
			expect_identifier(expect_mapped_string(colony_status_map, assign_variable_callback(entry.colonial))),
		"is_slave", ZERO_OR_ONE, expect_bool(assign_variable_callback(entry.slave)),
		"trade_goods", ZERO_OR_ONE,
			good_definition_manager.expect_good_definition_identifier_or_string(
				[&definition_manager, &entry](GoodDefinition const& rgo_good) ->bool {
					entry.rgo_production_type_nullable = definition_manager.get_economy_manager().get_production_type_manager().get_good_to_rgo_production_type()[rgo_good];
					if (entry.rgo_production_type_nullable == nullptr) {
						Logger::error(entry.province.get_identifier(), " has trade_goods ", rgo_good.get_identifier(), " which has no rgo production type defined.");
						//we expect the good to have an rgo production type
						//Victoria 2 treats this as null, but clearly the modder wanted there to be a good
						return false;
					}
					return true;
				},
				allow_empty_true, //could be explicitly setting trade_goods to null
				do_warn //could be typo in good identifier
			),
		"life_rating", ZERO_OR_ONE, expect_uint<ProvinceInstance::life_rating_t>(assign_variable_callback(entry.life_rating)),
		"terrain", ZERO_OR_ONE, terrain_type_manager.expect_terrain_type_identifier(
			assign_variable_callback_pointer_opt(entry.terrain_type)
		),
		"party_loyalty", ZERO_OR_MORE, [&ideology_manager, &entry](ast::NodeCPtr node) -> bool {
			Ideology const* ideology = nullptr;
			fixed_point_t amount = 0; /* PERCENTAGE_DECIMAL */

			bool ret = expect_dictionary_keys(
				"ideology", ONE_EXACTLY, ideology_manager.expect_ideology_identifier(
					assign_variable_callback_pointer(ideology)
				),
				"loyalty_value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(amount))
			)(node);
			if (ideology != nullptr) {
				ret &= map_callback(entry.party_loyalties, ideology)(amount);
			}
			return ret;
		},
		"state_building", ZERO_OR_MORE, [&building_type_manager, &entry](ast::NodeCPtr node) -> bool {
			BuildingType const* building_type = nullptr;
			uint8_t level = 0;

			bool ret = expect_dictionary_keys(
				"level", ONE_EXACTLY, expect_uint(assign_variable_callback(level)),
				"building", ONE_EXACTLY, building_type_manager.expect_building_type_identifier(
					assign_variable_callback_pointer(building_type)
				),
				"upgrade", ZERO_OR_ONE, success_callback /* Doesn't appear to have an effect */
			)(node);
			if (building_type != nullptr) {
				if (!building_type->is_in_province()) {
					ret &= map_callback(entry.state_buildings, building_type, true)(level);
				} else {
					Logger::error(
						"Attempted to add province building \"", building_type,
						"\" to state building list of province history for ", entry.get_province()
					);
					ret = false;
				}
			}
			return ret;
		}
	)(root);
}

void ProvinceHistoryManager::reserve_more_province_histories(size_t size) {
	if (locked) {
		Logger::error("Failed to reserve space for ", size, " provinces in ProvinceHistoryManager - already locked!");
	} else {
		reserve_more(province_histories, size);
	}
}

void ProvinceHistoryManager::lock_province_histories(MapDefinition const& map_definition, bool detailed_errors) {
	std::span<const ProvinceDefinition> provinces = map_definition.get_province_definitions();

	memory::vector<bool> province_checklist(provinces.size());
	for (auto [province, history_map] : mutable_iterator(province_histories)) {
		province_checklist[province->get_index() - 1] = true;

		history_map.sort_entries();
	}

	size_t missing = 0;
	for (size_t idx = 0; idx < province_checklist.size(); ++idx) {
		if (!province_checklist[idx]) {
			ProvinceDefinition const& province = provinces[idx];
			if (!province.is_water()) {
				if (detailed_errors) {
					Logger::warning("Province history missing for province: ", province.get_identifier());
				}
				missing++;
			}
		}
	}
	if (missing > 0) {
		Logger::warning("Province history is missing for ", missing, " provinces");
	}

	Logger::info("Locked province history registry after registering ", province_histories.size(), " items");
	locked = true;
}

bool ProvinceHistoryManager::is_locked() const {
	return locked;
}

ProvinceHistoryMap const* ProvinceHistoryManager::get_province_history(ProvinceDefinition const* province) const {
	if (province == nullptr) {
		Logger::error("Attempted to access history of null province");
		return nullptr;
	}
	decltype(province_histories)::const_iterator province_registry = province_histories.find(province);
	if (province_registry != province_histories.end()) {
		return &province_registry->second;
	} else {
		Logger::error("Attempted to access history of province ", province->get_identifier(), " but none has been defined!");
		return nullptr;
	}
}

ProvinceHistoryMap* ProvinceHistoryManager::_get_or_make_province_history(ProvinceDefinition const& province) {
	decltype(province_histories)::iterator it = province_histories.find(&province);
	if (it == province_histories.end()) {
		const std::pair<decltype(province_histories)::iterator, bool> result =
			province_histories.emplace(&province, ProvinceHistoryMap { province });
		if (result.second) {
			it = result.first;
		} else {
			Logger::error("Failed to create province history map for province ", province.get_identifier());
			return nullptr;
		}
	}
	return &it.value();
}

bool ProvinceHistoryManager::load_province_history_file(
	DefinitionManager const& definition_manager, ProvinceDefinition const& province, ast::NodeCPtr root
) {
	if (locked) {
		Logger::error(
			"Attempted to load province history file for ", province.get_identifier(),
			" after province history registry was locked!"
		);
		return false;
	}

	ProvinceHistoryMap* province_history = _get_or_make_province_history(province);
	if (province_history != nullptr) {
		return province_history->_load_history_file(definition_manager, root);
	} else {
		return false;
	}
}

bool ProvinceHistoryEntry::_load_province_pop_history(
	DefinitionManager const& definition_manager, ast::NodeCPtr root, bool *non_integer_size
) {
	PopManager const& pop_manager = definition_manager.get_pop_manager();
	RebelManager const& rebel_manager = definition_manager.get_politics_manager().get_rebel_manager();
	return pop_manager.expect_pop_type_dictionary_reserve_length(
		pops,
		[this, &pop_manager, &rebel_manager, non_integer_size](PopType const& pop_type, ast::NodeCPtr pop_node) -> bool {
			return pop_manager.load_pop_bases_into_vector(rebel_manager, pops, pop_type, pop_node, non_integer_size);
		}
	)(root);
}

bool ProvinceHistoryMap::_load_province_pop_history(
	DefinitionManager const& definition_manager, Date date, ast::NodeCPtr root, bool *non_integer_size
) {
	ProvinceHistoryEntry* entry = _get_or_make_entry(definition_manager, date);
	if (entry != nullptr) {
		return entry->_load_province_pop_history(definition_manager, root, non_integer_size);
	} else {
		return false;
	}
}

bool ProvinceHistoryManager::load_pop_history_file(
	DefinitionManager const& definition_manager, Date date, ast::NodeCPtr root, bool *non_integer_size
) {
	if (locked) {
		Logger::error("Attempted to load pop history file after province history registry was locked!");
		return false;
	}
	return definition_manager.get_map_definition().expect_province_definition_dictionary(
		[this, &definition_manager, date, non_integer_size](ProvinceDefinition const& province, ast::NodeCPtr node) -> bool {
			ProvinceHistoryMap* province_history = _get_or_make_province_history(province);
			if (province_history != nullptr) {
				return province_history->_load_province_pop_history(definition_manager, date, node, non_integer_size);
			} else {
				return false;
			}
		}
	)(root);
}
