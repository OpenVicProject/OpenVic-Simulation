#include "ProvinceHistory.hpp"

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

ProvinceHistoryEntry::ProvinceHistoryEntry(Province const& new_province, Date new_date)
	: HistoryEntry { new_date }, province { new_province } {}

ProvinceHistoryMap::ProvinceHistoryMap(Province const& new_province) : province { new_province } {}

std::unique_ptr<ProvinceHistoryEntry> ProvinceHistoryMap::_make_entry(Date date) const {
	return std::unique_ptr<ProvinceHistoryEntry> { new ProvinceHistoryEntry { province, date } };
}

bool ProvinceHistoryMap::_load_history_entry(
	GameManager const& game_manager, ProvinceHistoryEntry& entry, ast::NodeCPtr root
) {
	BuildingTypeManager const& building_type_manager = game_manager.get_economy_manager().get_building_type_manager();
	CountryManager const& country_manager = game_manager.get_country_manager();
	GoodManager const& good_manager = game_manager.get_economy_manager().get_good_manager();
	IdeologyManager const& ideology_manager = game_manager.get_politics_manager().get_ideology_manager();
	TerrainTypeManager const& terrain_type_manager = game_manager.get_map().get_terrain_type_manager();

	using enum Province::colony_status_t;
	static const string_map_t<Province::colony_status_t> colony_status_map {
		{ "0", STATE }, { "1", PROTECTORATE }, { "2", COLONY }
	};

	return expect_dictionary_keys_and_default(
		[this, &game_manager, &building_type_manager, &entry](
			std::string_view key, ast::NodeCPtr value) -> bool {
			// used for province buildings like forts or railroads
			BuildingType const* building_type = building_type_manager.get_building_type_by_identifier(key);
			if (building_type != nullptr) {
				if (building_type->is_in_province()) {
					return expect_uint<BuildingType::level_t>(map_callback(entry.province_buildings, building_type))(value);
				} else {
					Logger::error(
						"Attempted to add state building \"", building_type, "\" at top scope of province history for ",
						entry.get_province()
					);
					return false;
				}
			}

			return _load_history_sub_entry_callback(game_manager, entry.get_date(), value, key, value);
		},
		"owner", ZERO_OR_ONE,
			country_manager.expect_country_identifier(assign_variable_callback_pointer(entry.owner)),
		"controller", ZERO_OR_ONE,
			country_manager.expect_country_identifier(assign_variable_callback_pointer(entry.controller)),
		"add_core", ZERO_OR_MORE, country_manager.expect_country_identifier(vector_callback_pointer(entry.add_cores)),
		"remove_core", ZERO_OR_MORE, country_manager.expect_country_identifier(
			vector_callback_pointer(entry.remove_cores)
		),
		"colonial", ZERO_OR_ONE,
			expect_identifier(expect_mapped_string(colony_status_map, assign_variable_callback(entry.colonial))),
		"colony", ZERO_OR_ONE,
			expect_identifier(expect_mapped_string(colony_status_map, assign_variable_callback(entry.colonial))),
		"is_slave", ZERO_OR_ONE, expect_bool(assign_variable_callback(entry.slave)),
		"trade_goods", ZERO_OR_ONE, good_manager.expect_good_identifier(assign_variable_callback_pointer(entry.rgo)),
		"life_rating", ZERO_OR_ONE, expect_uint<Province::life_rating_t>(assign_variable_callback(entry.life_rating)),
		"terrain", ZERO_OR_ONE, terrain_type_manager.expect_terrain_type_identifier(
			assign_variable_callback_pointer(entry.terrain_type)
		),
		"party_loyalty", ZERO_OR_MORE, [&ideology_manager, &entry](ast::NodeCPtr node) -> bool {
			Ideology const* ideology = nullptr;
			fixed_point_t amount = 0; // percent I do believe

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
				"upgrade", ZERO_OR_ONE, success_callback // doesn't appear to have an effect
			)(node);
			if (building_type != nullptr) {
				if (!building_type->is_in_province()) {
					ret &= map_callback(entry.state_buildings, building_type)(level);
				} else {
					Logger::error(
						"Attempted to add province building \"", building_type, "\" to state building list of province history for ",
						entry.get_province()
					);
					ret = false;
				}
			}
			return ret;
		}
	)(root);
}

void ProvinceHistoryManager::lock_province_histories(Map const& map, bool detailed_errors) {
	std::vector<bool> province_checklist(map.get_province_count());
	for (decltype(province_histories)::value_type const& entry : province_histories) {
		province_checklist[entry.first->get_index() - 1] = true;
	}

	size_t missing = 0;
	for (size_t idx = 0; idx < province_checklist.size(); ++idx) {
		if (!province_checklist[idx]) {
			Province const& province = *map.get_province_by_index(idx + 1);
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

ProvinceHistoryMap const* ProvinceHistoryManager::get_province_history(Province const* province) const {
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

ProvinceHistoryMap* ProvinceHistoryManager::_get_or_make_province_history(Province const& province) {
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
	GameManager const& game_manager, Province const& province, ast::NodeCPtr root
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
		return province_history->_load_history_file(game_manager, root);
	} else {
		return false;
	}
}

bool ProvinceHistoryEntry::_load_province_pop_history(
	GameManager const& game_manager, ast::NodeCPtr root, bool *non_integer_size
) {
	PopManager const& pop_manager = game_manager.get_pop_manager();
	RebelManager const& rebel_manager = game_manager.get_politics_manager().get_rebel_manager();
	return pop_manager.expect_pop_type_dictionary_reserve_length(
		pops,
		[this, &pop_manager, &rebel_manager, non_integer_size](PopType const& pop_type, ast::NodeCPtr pop_node) -> bool {
			return pop_manager.load_pop_into_vector(rebel_manager, pops, pop_type, pop_node, non_integer_size);
		}
	)(root);
}

bool ProvinceHistoryMap::_load_province_pop_history(
	GameManager const& game_manager, Date date, ast::NodeCPtr root, bool *non_integer_size
) {
	ProvinceHistoryEntry* entry = _get_or_make_entry(game_manager, date);
	if (entry != nullptr) {
		return entry->_load_province_pop_history(game_manager, root, non_integer_size);
	} else {
		return false;
	}
}

bool ProvinceHistoryManager::load_pop_history_file(
	GameManager const& game_manager, Date date, ast::NodeCPtr root, bool *non_integer_size
) {
	if (locked) {
		Logger::error("Attempted to load pop history file after province history registry was locked!");
		return false;
	}
	return game_manager.get_map().expect_province_dictionary(
		[this, &game_manager, date, non_integer_size](Province const& province, ast::NodeCPtr node) -> bool {
			ProvinceHistoryMap* province_history = _get_or_make_province_history(province);
			if (province_history != nullptr) {
				return province_history->_load_province_pop_history(game_manager, date, node, non_integer_size);
			} else {
				return false;
			}
		}
	)(root);
}
