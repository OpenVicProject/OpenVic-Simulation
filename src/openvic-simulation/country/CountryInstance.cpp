#include "CountryInstance.hpp"

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/history/CountryHistory.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/research/Invention.hpp"
#include "openvic-simulation/research/Technology.hpp"

using namespace OpenVic;

static constexpr colour_t ERROR_COLOUR = colour_t::from_integer(0xFF0000);

CountryInstance::CountryInstance(
	CountryDefinition const* new_country_definition,
	decltype(technologies)::keys_t const& technology_keys,
	decltype(inventions)::keys_t const& invention_keys,
	decltype(upper_house)::keys_t const& ideology_keys,
	decltype(pop_type_distribution)::keys_t const& pop_type_keys
) : /* Main attributes */
	country_definition { new_country_definition },
	colour { ERROR_COLOUR },
	capital { nullptr },
	country_flags {},
	civilised { false },
	releasable_vassal { true },
	owned_provinces {},
	controlled_provinces {},
	core_provinces {},
	states {},

	/* Production */

	/* Budget */
	cash_stockpile { 0 },

	/* Technology */
	technologies { &technology_keys },
	inventions { &invention_keys },
	current_research { nullptr },
	invested_research_points { 0 },
	expected_completion_date {},
	research_point_stockpile { 0 },
	daily_research_points { 0 },
	national_literacy { 0 },
	tech_school { nullptr },

	/* Politics */
	national_value { nullptr },
	government_type { nullptr },
	last_election {},
	ruling_party { nullptr },
	upper_house { &ideology_keys },
	reforms {},
	suppression_points { 0 },
	infamy { 0 },
	plurality { 0 },
	revanchism { 0 },

	/* Population */
	primary_culture { nullptr },
	accepted_cultures {},
	religion { nullptr },
	total_population { 0 },
	national_consciousness { 0 },
	national_militancy { 0 },
	pop_type_distribution { &pop_type_keys },
	national_focus_capacity { 0 },

	/* Trade */

	/* Diplomacy */
	total_rank { 0 },
	prestige { 0 },
	prestige_rank { 0 },
	industrial_power { 0 },
	industrial_rank { 0 },
	military_power { 0 },
	military_rank { 0 },
	diplomatic_points { 0 },

	/* Military */
	generals {},
	admirals {},
	armies {},
	navies {},
	regiment_count { 0 },
	mobilisation_regiment_potential { 0 },
	ship_count { 0 },
	total_consumed_ship_supply { 0 },
	max_ship_supply { 0 },
	leadership_points { 0 },
	war_exhaustion { 0 } {}

std::string_view CountryInstance::get_identifier() const {
	return country_definition->get_identifier();
}

bool CountryInstance::set_country_flag(std::string_view flag, bool warn) {
	if (flag.empty()) {
		Logger::error("Attempted to set empty country flag for country ", get_identifier());
		return false;
	}
	if (!country_flags.emplace(flag).second && warn) {
		Logger::warning(
			"Attempted to set country flag \"", flag, "\" for country ", get_identifier(), ": already set!"
		);
	}
	return true;
}

bool CountryInstance::clear_country_flag(std::string_view flag, bool warn) {
	if (flag.empty()) {
		Logger::error("Attempted to clear empty country flag from country ", get_identifier());
		return false;
	}
	if (country_flags.erase(flag) == 0 && warn) {
		Logger::warning(
			"Attempted to clear country flag \"", flag, "\" from country ", get_identifier(), ": not set!"
		);
	}
	return true;
}

#define ADD_AND_REMOVE(item) \
	bool CountryInstance::add_##item(std::remove_pointer_t<decltype(item##s)::value_type>& new_item) { \
		if (!item##s.emplace(&new_item).second) { \
			Logger::error( \
				"Attempted to add " #item " \"", new_item.get_identifier(), "\" to country ", get_identifier(), \
				": already present!" \
			); \
			return false; \
		} \
		return true; \
	} \
	bool CountryInstance::remove_##item(std::remove_pointer_t<decltype(item##s)::value_type>& item_to_remove) { \
		if (item##s.erase(&item_to_remove) == 0) { \
			Logger::error( \
				"Attempted to remove " #item " \"", item_to_remove.get_identifier(), "\" from country ", get_identifier(), \
				": not present!" \
			); \
			return false; \
		} \
		return true; \
	}

ADD_AND_REMOVE(owned_province)
ADD_AND_REMOVE(controlled_province)
ADD_AND_REMOVE(core_province)
ADD_AND_REMOVE(state)
ADD_AND_REMOVE(accepted_culture)

#undef ADD_AND_REMOVE

bool CountryInstance::set_upper_house(Ideology const* ideology, fixed_point_t popularity) {
	if (ideology != nullptr) {
		upper_house[*ideology] = popularity;
		return true;
	} else {
		Logger::error("Trying to set null ideology in upper house of ", get_identifier());
		return false;
	}
}

bool CountryInstance::add_reform(Reform const* new_reform) {
	if (std::find(reforms.begin(), reforms.end(), new_reform) != reforms.end()) {
		Logger::warning(
			"Attempted to add reform \"", new_reform, "\" to country ", get_identifier(), ": already present!"
		);
		return false;
	}
	reforms.push_back(new_reform);
	return true;
}

bool CountryInstance::remove_reform(Reform const* reform_to_remove) {
	auto existing_entry = std::find(reforms.begin(), reforms.end(), reform_to_remove);
	if (existing_entry == reforms.end()) {
		Logger::warning(
			"Attempted to remove reform \"", reform_to_remove, "\" from country ", get_identifier(), ": not present!"
		);
		return false;
	}
	reforms.erase(existing_entry);
	return true;
}

template<UnitType::branch_t Branch>
bool CountryInstance::add_unit_instance_group(UnitInstanceGroup<Branch>& group) {
	if (get_unit_instance_groups<Branch>().emplace(static_cast<UnitInstanceGroupBranched<Branch>*>(&group)).second) {
		return true;
	} else {
		Logger::error(
			"Trying to add already-existing ", Branch == UnitType::branch_t::LAND ? "army" : "navy", " ",
			group.get_name(), " to country ", get_identifier()
		);
		return false;
	}
}

template<UnitType::branch_t Branch>
bool CountryInstance::remove_unit_instance_group(UnitInstanceGroup<Branch>& group) {
	if (get_unit_instance_groups<Branch>().erase(static_cast<UnitInstanceGroupBranched<Branch>*>(&group)) > 0) {
		return true;
	} else {
		Logger::error(
			"Trying to remove non-existent ", Branch == UnitType::branch_t::LAND ? "army" : "navy", " ",
			group.get_name(), " from country ", get_identifier()
		);
		return false;
	}
}

template bool CountryInstance::add_unit_instance_group(UnitInstanceGroup<UnitType::branch_t::LAND>&);
template bool CountryInstance::add_unit_instance_group(UnitInstanceGroup<UnitType::branch_t::NAVAL>&);
template bool CountryInstance::remove_unit_instance_group(UnitInstanceGroup<UnitType::branch_t::LAND>&);
template bool CountryInstance::remove_unit_instance_group(UnitInstanceGroup<UnitType::branch_t::NAVAL>&);

template<UnitType::branch_t Branch>
void CountryInstance::add_leader(LeaderBranched<Branch>&& leader) {
	get_leaders<Branch>().emplace(std::move(leader));
}

template<UnitType::branch_t Branch>
bool CountryInstance::remove_leader(LeaderBranched<Branch> const* leader) {
	plf::colony<LeaderBranched<Branch>>& leaders = get_leaders<Branch>();
	const auto it = leaders.get_iterator(leader);
	if (it != leaders.end()) {
		leaders.erase(it);
		return true;
	}

	Logger::error(
		"Trying to remove non-existent ", Branch == UnitType::branch_t::LAND ? "general" : "admiral", " ",
		leader != nullptr ? leader->get_name() : "NULL", " from country ", get_identifier()
	);
	return false;
}

template void CountryInstance::add_leader(LeaderBranched<UnitType::branch_t::LAND>&&);
template void CountryInstance::add_leader(LeaderBranched<UnitType::branch_t::NAVAL>&&);
template bool CountryInstance::remove_leader(LeaderBranched<UnitType::branch_t::LAND> const*);
template bool CountryInstance::remove_leader(LeaderBranched<UnitType::branch_t::NAVAL> const*);

bool CountryInstance::apply_history_to_country(CountryHistoryEntry const* entry, MapInstance& map_instance) {
	if (entry == nullptr) {
		Logger::error("Trying to apply null country history to ", get_identifier());
		return false;
	}

	constexpr auto set_optional = []<typename T>(T& target, std::optional<T> const& source) {
		if (source) {
			target = *source;
		}
	};

	bool ret = true;

	set_optional(primary_culture, entry->get_primary_culture());
	for (Culture const* culture : entry->get_accepted_cultures()) {
		ret &= add_accepted_culture(*culture);
	}
	set_optional(religion, entry->get_religion());
	set_optional(ruling_party, entry->get_ruling_party());
	set_optional(last_election, entry->get_last_election());
	ret &= upper_house.copy(entry->get_upper_house());
	if (entry->get_capital()) {
		capital = &map_instance.get_province_instance_from_definition(**entry->get_capital());
	}
	set_optional(government_type, entry->get_government_type());
	set_optional(plurality, entry->get_plurality());
	set_optional(national_value, entry->get_national_value());
	set_optional(civilised, entry->is_civilised());
	set_optional(prestige, entry->get_prestige());
	for (Reform const* reform : entry->get_reforms()) {
		ret &= add_reform(reform);
	}
	set_optional(tech_school, entry->get_tech_school());
	constexpr auto set_bool_map_to_indexed_map =
		[]<typename T>(IndexedMap<T, bool>& target, ordered_map<T const*, bool> source) {
			for (auto const& [key, value] : source) {
				target[*key] = value;
			}
		};
	set_bool_map_to_indexed_map(technologies, entry->get_technologies());
	set_bool_map_to_indexed_map(inventions, entry->get_inventions());
	// entry->get_foreign_investment();

	// These need to be applied to pops
	// entry->get_consciousness();
	// entry->get_nonstate_consciousness();
	// entry->get_literacy();
	// entry->get_nonstate_culture_literacy();

	set_optional(releasable_vassal, entry->is_releasable_vassal());
	// entry->get_colonial_points();
	for (std::string const& flag : entry->get_country_flags()) {
		ret &= set_country_flag(flag, true);
	}
	for (std::string const& flag : entry->get_global_flags()) {
		// TODO - set global flag
	}
	// entry->get_government_flag_overrides();
	for (Decision const* decision : entry->get_decisions()) {
		// TODO - take decision
	}

	return ret;
}

void CountryInstance::_update_production() {

}

void CountryInstance::_update_budget() {

}

void CountryInstance::_update_technology() {

}

void CountryInstance::_update_politics() {

}

void CountryInstance::_update_population() {
	total_population = 0;
	national_literacy = 0;
	national_consciousness = 0;
	national_militancy = 0;
	pop_type_distribution.clear();

	for (auto const& state : states) {
		total_population += state->get_total_population();

		// TODO - change casting if Pop::pop_size_t changes type
		const fixed_point_t state_population = fixed_point_t::parse(state->get_total_population());
		national_literacy += state->get_average_literacy() * state_population;
		national_consciousness += state->get_average_consciousness() * state_population;
		national_militancy += state->get_average_militancy() * state_population;

		pop_type_distribution += state->get_pop_type_distribution();
	}

	if (total_population > 0) {
		national_literacy /= total_population;
		national_consciousness /= total_population;
		national_militancy /= total_population;
	}

	// TODO - update national focus capacity
}

void CountryInstance::_update_trade() {
	// TODO - update total amount of each good exported and imported
}

void CountryInstance::_update_diplomacy() {
	// TODO - update prestige, industrial_power, military_power (ranks will be updated after all countries have calculated their scores)
	// TODO - update diplomatic points and colonial power
}

void CountryInstance::_update_military() {
	regiment_count = 0;

	for (ArmyInstance const* army : armies) {
		regiment_count += army->get_unit_count();
	}

	ship_count = 0;
	total_consumed_ship_supply = 0;

	for (NavyInstance const* navy : navies) {
		ship_count += navy->get_unit_count();
		total_consumed_ship_supply += navy->get_total_consumed_supply();
	}

	// TODO - update mobilisation_regiment_potential, max_ship_supply, leadership_points, war_exhaustion
}

void CountryInstance::update_gamestate() {
	if (country_definition != nullptr) {
		const CountryDefinition::government_colour_map_t::const_iterator it =
			country_definition->get_alternative_colours().find(government_type);

		if (it != country_definition->get_alternative_colours().end()) {
			colour = it->second;
		} else {
			colour = country_definition->get_colour();
		}
	} else {
		colour = ERROR_COLOUR;
	}

	// Order of updates might need to be changed/functions split up to account for dependencies
	_update_production();
	_update_budget();
	_update_technology();
	_update_politics();
	_update_population();
	_update_trade();
	_update_diplomacy();
	_update_military();
}

void CountryInstance::tick() {

}

CountryInstance& CountryInstanceManager::get_country_instance_from_definition(CountryDefinition const& country) {
	return country_instances.get_items()[country.get_index()];
}

CountryInstance const& CountryInstanceManager::get_country_instance_from_definition(CountryDefinition const& country) const {
	return country_instances.get_items()[country.get_index()];
}

bool CountryInstanceManager::generate_country_instances(
	CountryDefinitionManager const& country_definition_manager,
	decltype(CountryInstance::technologies)::keys_t const& technology_keys,
	decltype(CountryInstance::inventions)::keys_t const& invention_keys,
	decltype(CountryInstance::upper_house)::keys_t const& ideology_keys,
	decltype(CountryInstance::pop_type_distribution)::keys_t const& pop_type_keys
) {
	reserve_more(country_instances, country_definition_manager.get_country_definition_count());

	for (CountryDefinition const& country_definition : country_definition_manager.get_country_definitions()) {
		country_instances.add_item({ &country_definition, technology_keys, invention_keys, ideology_keys, pop_type_keys });
	}

	return true;
}

bool CountryInstanceManager::apply_history_to_countries(
	CountryHistoryManager const& history_manager, Date date, UnitInstanceManager& unit_instance_manager,
	MapInstance& map_instance
) {
	bool ret = true;

	for (CountryInstance& country_instance : country_instances.get_items()) {
		if (!country_instance.get_country_definition()->is_dynamic_tag()) {
			CountryHistoryMap const* history_map =
				history_manager.get_country_history(country_instance.get_country_definition());

			if (history_map != nullptr) {
				CountryHistoryEntry const* oob_history_entry = nullptr;

				for (CountryHistoryEntry const* entry : history_map->get_entries_up_to(date)) {
					ret &= country_instance.apply_history_to_country(entry, map_instance);

					if (entry->get_inital_oob()) {
						oob_history_entry = entry;
					}
				}

				if (oob_history_entry != nullptr) {
					ret &= unit_instance_manager.generate_deployment(
						map_instance, country_instance, *oob_history_entry->get_inital_oob()
					);
				}
			} else {
				Logger::error("Country ", country_instance.get_identifier(), " has no history!");
				ret = false;
			}
		}
	}

	return ret;
}

void CountryInstanceManager::update_gamestate() {
	for (CountryInstance& country : country_instances.get_items()) {
		country.update_gamestate();
	}
}

void CountryInstanceManager::tick() {
	for (CountryInstance& country : country_instances.get_items()) {
		country.tick();
	}
}
