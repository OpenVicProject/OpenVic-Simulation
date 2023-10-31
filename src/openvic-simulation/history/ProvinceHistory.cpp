#include "ProvinceHistory.hpp"

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

ProvinceHistory::ProvinceHistory(
	Country const* new_owner, Country const* new_controller, Province::colony_status_t new_colonial, bool new_slave,
	std::vector<Country const*>&& new_cores, Good const* new_rgo, Province::life_rating_t new_life_rating,
	TerrainType const* new_terrain_type, building_level_map_t&& new_buildings,
	decimal_map_t<Ideology const*>&& new_party_loyalties
) : owner { new_owner }, controller { new_controller }, colonial { new_colonial }, slave { new_slave },
	cores { std::move(new_cores) }, rgo { new_rgo }, life_rating { new_life_rating }, terrain_type { new_terrain_type },
	buildings { std::move(new_buildings) }, party_loyalties { std::move(new_party_loyalties) } {}

Country const* ProvinceHistory::get_owner() const {
	return owner;
}

Country const* ProvinceHistory::get_controller() const {
	return controller;
}

Province::colony_status_t ProvinceHistory::get_colony_status() const {
	return colonial;
}

bool ProvinceHistory::is_slave() const {
	return slave;
}

std::vector<Country const*> const& ProvinceHistory::get_cores() const {
	return cores;
}

bool ProvinceHistory::is_core_of(Country const* country) const {
	return std::find(cores.begin(), cores.end(), country) != cores.end();
}

Good const* ProvinceHistory::get_rgo() const {
	return rgo;
}

Province::life_rating_t ProvinceHistory::get_life_rating() const {
	return life_rating;
}

TerrainType const* ProvinceHistory::get_terrain_type() const {
	return terrain_type;
}

ProvinceHistory::building_level_map_t const& ProvinceHistory::get_buildings() const {
	return buildings;
}

decimal_map_t<Ideology const*> const& ProvinceHistory::get_party_loyalties() const {
	return party_loyalties;
}

bool ProvinceHistoryManager::add_province_history_entry(
	Province const* province, Date date, Country const* owner, Country const* controller,
	std::optional<Province::colony_status_t>&& colonial, std::optional<bool>&& slave, std::vector<Country const*>&& cores,
	std::vector<Country const*>&& remove_cores, Good const* rgo, std::optional<Province::life_rating_t>&& life_rating,
	TerrainType const* terrain_type, std::optional<ProvinceHistory::building_level_map_t>&& buildings,
	std::optional<decimal_map_t<Ideology const*>>&& party_loyalties
) {
	if (locked) {
		Logger::error("Cannot add new history entry to province history registry: locked!");
		return false;
	}

	/* combine duplicate histories, priority to current (defined later) */
	auto& province_registry = province_histories[province];
	const auto existing_entry = province_registry.find(date);

	if (existing_entry != province_registry.end()) {
		if (owner != nullptr) {
			existing_entry->second.owner = owner;
		}
		if (controller != nullptr) {
			existing_entry->second.controller = controller;
		}
		if (rgo != nullptr) {
			existing_entry->second.rgo = rgo;
		}
		if (terrain_type != nullptr) {
			existing_entry->second.terrain_type = terrain_type;
		}
		if (colonial) {
			existing_entry->second.colonial = *colonial;
		}
		if (slave) {
			existing_entry->second.slave = *slave;
		}
		if (life_rating) {
			existing_entry->second.life_rating = *life_rating;
		}
		if (buildings) {
			existing_entry->second.buildings = std::move(*buildings);
		}
		if (party_loyalties) {
			existing_entry->second.party_loyalties = std::move(*party_loyalties);
		}
		// province history cores are additive
		existing_entry->second.cores.insert(existing_entry->second.cores.end(), cores.begin(), cores.end());
		for (Country const* which : remove_cores) {
			const auto core = std::find(cores.begin(), cores.end(), which);
			if (core == cores.end()) {
				Logger::error(
					"In history of province ", province->get_identifier(), " tried to remove nonexistant core of country: ",
					which->get_identifier(), " at date ", date.to_string()
				);
				return false;
			}
			existing_entry->second.cores.erase(core);
		}
	} else {
		province_registry.emplace(date, ProvinceHistory {
			owner, controller, *colonial, *slave, std::move(cores), rgo, *life_rating,
			terrain_type, std::move(*buildings), std::move(*party_loyalties)
		});
	}
	return true;
}

void ProvinceHistoryManager::lock_province_histories() {
	for (auto const& entry : province_histories) {
		if (entry.second.size() == 0) {
			Logger::error(
				"Attempted to lock province histories - province ", entry.first->get_identifier(), " has no history entries!"
			);
		}
	}
	Logger::info("Locked province history registry after registering ", province_histories.size(), " items");
	locked = true;
}

bool ProvinceHistoryManager::is_locked() const {
	return locked;
}

ProvinceHistory const* ProvinceHistoryManager::get_province_history(Province const* province, Date entry) const {
	Date closest_entry;
	auto province_registry = province_histories.find(province);

	if (province_registry == province_histories.end()) {
		Logger::error("Attempted to access history of undefined province ", province->get_identifier());
		return nullptr;
	}

	for (auto const& current : province_registry->second) {
		if (current.first == entry) {
			return &current.second;
		}
		if (current.first > entry) {
			continue;
		}
		if (current.first > closest_entry && current.first < entry) {
			closest_entry = current.first;
		}
	}

	auto entry_registry = province_registry->second.find(closest_entry);
	if (entry_registry != province_registry->second.end()) {
		return &entry_registry->second;
	}
	/* warned about lack of entries earlier, return nullptr */
	return nullptr;
}

inline ProvinceHistory const* ProvinceHistoryManager::get_province_history(
	Province const* province, Bookmark const* bookmark
) const {
	return get_province_history(province, bookmark->get_date());
}

inline bool ProvinceHistoryManager::_load_province_history_entry(
	GameManager const& game_manager, Province const& province, Date date, ast::NodeCPtr root,
	bool is_base_entry
) {
	BuildingManager const& building_manager = game_manager.get_economy_manager().get_building_manager();
	CountryManager const& country_manager = game_manager.get_country_manager();
	GoodManager const& good_manager = game_manager.get_economy_manager().get_good_manager();
	IdeologyManager const& ideology_manager = game_manager.get_politics_manager().get_ideology_manager();
	TerrainTypeManager const& terrain_type_manager = game_manager.get_map().get_terrain_type_manager();

	Country const* owner = nullptr;
	Country const* controller = nullptr;
	std::vector<Country const*> cores {};
	std::vector<Country const*> remove_cores {};
	Good const* rgo = nullptr;
	std::optional<Province::colony_status_t> colonial;
	std::optional<Province::life_rating_t> life_rating;
	std::optional<bool> slave;
	TerrainType const* terrain_type = nullptr;
	std::optional<ProvinceHistory::building_level_map_t> buildings;
	std::optional<decimal_map_t<Ideology const*>> party_loyalties;

	using enum Province::colony_status_t;
	static const string_map_t<Province::colony_status_t> colony_status_map {
		{ "0", STATE }, { "1", PROTECTORATE }, { "2", COLONY }
	};

	bool ret = expect_dictionary_keys_and_default(
		[&building_manager, &buildings, date, is_base_entry](std::string_view key, ast::NodeCPtr value) -> bool {
			// used for province buildings like forts or railroads
			Building const* building = building_manager.get_building_by_identifier(key);
			if (building != nullptr) {
				return expect_uint<Building::level_t>([&buildings, building](Building::level_t level) -> bool {
					if(!buildings.has_value())
						buildings = decltype(buildings)::value_type {};
					buildings->emplace(building, level);
					return true;
				})(value);
			}

			/* Date blocks are skipped here (they get their own invocation of _load_province_history_entry) */
			bool is_date = false;
			const Date sub_date { Date::from_string(key, &is_date, true) };
			if (is_date) {
				if (is_base_entry) {
					return true;
				} else {
					Logger::error(
						"Province history nested multiple levels deep! ", sub_date, " is inside ", date,
						" (Any date blocks within a date block are ignored)"
					);
					return false;
				}
			}

			return key_value_invalid_callback(key, value);
		},
		"owner", ZERO_OR_ONE, country_manager.expect_country_identifier(assign_variable_callback_pointer(owner)),
		"controller", ZERO_OR_ONE, country_manager.expect_country_identifier(assign_variable_callback_pointer(controller)),
		"add_core", ZERO_OR_MORE, country_manager.expect_country_identifier(
			[&cores](Country const& core) -> bool {
				cores.push_back(&core);
				return true;
			}
		),
		"remove_core", ZERO_OR_MORE, country_manager.expect_country_identifier(
			[&remove_cores](Country const& core) -> bool {
				remove_cores.push_back(&core);
				return true;
			}
		),
		"colonial", ZERO_OR_ONE,
			expect_identifier(expect_mapped_string(colony_status_map, assign_variable_callback(colonial))),
		"colony", ZERO_OR_ONE, expect_identifier(expect_mapped_string(colony_status_map, assign_variable_callback(colonial))),
		"is_slave", ZERO_OR_ONE, expect_bool(assign_variable_callback(slave)),
		"trade_goods", ZERO_OR_ONE, good_manager.expect_good_identifier(assign_variable_callback_pointer(rgo)),
		"life_rating", ZERO_OR_ONE, expect_uint<Province::life_rating_t>(assign_variable_callback(life_rating)),
		"terrain", ZERO_OR_ONE, terrain_type_manager.expect_terrain_type_identifier(
			assign_variable_callback_pointer(terrain_type)
		),
		"party_loyalty", ZERO_OR_MORE, [&ideology_manager, &party_loyalties](ast::NodeCPtr node) -> bool {
			Ideology const* ideology = nullptr;
			fixed_point_t amount = 0; // percent I do believe

			const bool ret = expect_dictionary_keys(
				"ideology", ONE_EXACTLY, ideology_manager.expect_ideology_identifier(
					assign_variable_callback_pointer(ideology)
				),
				"loyalty_value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(amount))
			)(node);
			if(!party_loyalties.has_value())
				party_loyalties = decltype(party_loyalties)::value_type {};
			party_loyalties->emplace(ideology, amount);
			return ret;
		},
		"state_building", ZERO_OR_MORE, [&building_manager, &buildings](ast::NodeCPtr node) -> bool {
			Building const* building = nullptr;
			uint8_t level = 0;

			const bool ret = expect_dictionary_keys(
				"level", ONE_EXACTLY, expect_uint(assign_variable_callback(level)),
				"building", ONE_EXACTLY, building_manager.expect_building_identifier(
					assign_variable_callback_pointer(building)
				),
				"upgrade", ZERO_OR_ONE, success_callback // doesn't appear to have an effect
			)(node);
			if(!buildings.has_value())
				buildings = decltype(buildings)::value_type {};
			buildings->emplace(building, level);
			return ret;
		}
	)(root);

	ret &= add_province_history_entry(
		&province, date, owner, controller, std::move(colonial), std::move(slave), std::move(cores), std::move(remove_cores),
		rgo, std::move(life_rating), terrain_type, std::move(buildings), std::move(party_loyalties)
	);
	return ret;
}

bool ProvinceHistoryManager::load_province_history_file(
	GameManager const& game_manager, Province const& province, ast::NodeCPtr root
) {
	bool ret = _load_province_history_entry(
		game_manager, province, game_manager.get_define_manager().get_start_date(), root, true
	);

	ret &= expect_dictionary(
		[this, &game_manager, &province, end_date = game_manager.get_define_manager().get_end_date()](
			std::string_view key, ast::NodeCPtr value) -> bool {
			bool is_date = false;
			const Date entry = Date::from_string(key, &is_date, true);
			if (!is_date) {
				return true;
			}

			if (entry > end_date) {
				Logger::error(
					"History entry ", entry, " of province ", province.get_identifier(),
					" defined after defined end date ", end_date
				);
				return false;
			}

			return _load_province_history_entry(game_manager, province, entry, value, false);
		}
	)(root);

	return ret;
}
