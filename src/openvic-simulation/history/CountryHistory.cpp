#include "CountryHistory.hpp"

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/core/FormatValidate.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

CountryHistoryEntry::CountryHistoryEntry(
	CountryDefinition const& new_country, Date new_date, decltype(upper_house_proportion_by_ideology)::keys_span_type ideology_keys,
	decltype(flag_overrides_by_government_type)::keys_span_type government_type_keys
) : HistoryEntry { new_date }, country { new_country }, upper_house_proportion_by_ideology { ideology_keys },
	flag_overrides_by_government_type { government_type_keys } {}

fixed_point_t CountryHistoryEntry::get_upper_house_proportion_by_ideology(Ideology const& key) const {
	return upper_house_proportion_by_ideology.at(key);
}

GovernmentType const* CountryHistoryEntry::get_flag_overrides_by_government_type(GovernmentType const& key) const {
	return flag_overrides_by_government_type.at(key);
}

CountryHistoryMap::CountryHistoryMap(
	CountryDefinition const& new_country, decltype(ideology_keys) new_ideology_keys,
	decltype(government_type_keys) new_government_type_keys
) : country { new_country }, ideology_keys { new_ideology_keys }, government_type_keys { new_government_type_keys } {}

memory::unique_ptr<CountryHistoryEntry> CountryHistoryMap::_make_entry(Date date) const {
	return memory::make_unique<CountryHistoryEntry>(country, date, ideology_keys, government_type_keys);
}

static constexpr auto _flag_callback(string_map_t<bool>& flags, bool value) {
	return [&flags, value](std::string_view flag) -> bool {
		auto [it, successful] = flags.emplace(flag, value);
		if (!successful) {
			it.value() = value;
		}
		return true;
	};
}

bool CountryHistoryMap::_load_history_entry(
	DefinitionManager const& definition_manager, Dataloader const& dataloader, DeploymentManager& deployment_manager,
	CountryHistoryEntry& entry, ast::NodeCPtr root
) {
	PoliticsManager const& politics_manager = definition_manager.get_politics_manager();
	IssueManager const& issue_manager = politics_manager.get_issue_manager();
	CultureManager const& culture_manager = definition_manager.get_pop_manager().get_culture_manager();
	CountryDefinitionManager const& country_definition_manager = definition_manager.get_country_definition_manager();
	TechnologyManager const& technology_manager = definition_manager.get_research_manager().get_technology_manager();
	InventionManager const& invention_manager = definition_manager.get_research_manager().get_invention_manager();
	DecisionManager const& decision_manager = definition_manager.get_decision_manager();

	const auto accepted_culture_instruction = [&entry](bool add) {
		return [&entry, add](Culture const& culture) -> bool {
			const auto it = entry.accepted_cultures.find(&culture);
			if (it == entry.accepted_cultures.end()) {
				// No current culture instruction
				entry.accepted_cultures.emplace(&culture, add);
				return true;
			} else if (it->second == add) {
				// Desired culture instruction already exists
				spdlog::warn_s(
					"Duplicate attempt to {} accepted culture {} {} country history of {}",
					add ? "add" : "remove", add ? "to" : "from", culture, entry.country
				);
				return true;
			} else {
				// Opposite culture instruction exists
				entry.accepted_cultures.erase(it);
				spdlog::warn_s(
					"Attempted to {} accepted culture {} {} country history of {} after previously {} it",
					add ? "add" : "remove",
					culture,
					add ? "to" : "from",
					entry.country,
					add ? "removing" : "adding"
				);
				return true;
			}
		};
	};

	return expect_dictionary_keys_and_default_map(
		[
			this, &definition_manager, &dataloader,
			&deployment_manager, &issue_manager,
			&technology_manager, &invention_manager,
			&country_definition_manager, &entry
		](
			template_key_map_t<StringMapCaseSensitive> const& key_map,
			std::string_view key,
			ast::NodeCPtr value
		) -> bool {
			ReformGroup const* reform_group = issue_manager.get_reform_group_by_identifier(key);
			if (reform_group != nullptr) {
				return issue_manager.expect_reform_identifier([&entry, reform_group](Reform const& reform) -> bool {
					if (&reform.get_reform_group() != reform_group) {
						spdlog::warn_s(
							"Listing {} as belonging to the reform group {} when it actually belongs to {} in history of {}",
							reform, *reform_group, reform.get_reform_group(), entry.country
						);
					}
					return set_callback_pointer(entry.reforms)(reform);
				})(value);
			}

			{
				Technology const* technology = technology_manager.get_technology_by_identifier(key);
				if (technology != nullptr) {
					return expect_uint<decltype(entry.technologies)::mapped_type>(
						[&entry, technology](decltype(entry.technologies)::mapped_type value) -> bool {
							if (value > 1) {
								spdlog::warn_s(
									"Technology {} is applied multiple times in history of country {}",
									*technology, entry.country
								);
							}
							return map_callback(entry.technologies, technology)(value);
						}
					)(value);
				}
			}

			{
				Invention const* invention = invention_manager.get_invention_by_identifier(key);
				if (invention != nullptr) {
					return expect_bool(map_callback(entry.inventions, invention))(value);
				}
			}

			return _load_history_sub_entry_callback(
				definition_manager, dataloader, deployment_manager, entry.get_date(), value, key_map, key, value
			);
		},
		"capital", ZERO_OR_ONE, definition_manager.get_map_definition().expect_province_definition_identifier(
			assign_variable_callback_pointer_opt(entry.capital)
		),
		"primary_culture", ZERO_OR_ONE,
			culture_manager.expect_culture_identifier(assign_variable_callback_pointer_opt(entry.primary_culture)),
		"culture", ZERO_OR_MORE, culture_manager.expect_culture_identifier(accepted_culture_instruction(true)),
		"remove_culture", ZERO_OR_MORE, culture_manager.expect_culture_identifier(accepted_culture_instruction(false)),
		"religion", ZERO_OR_ONE, definition_manager.get_pop_manager().get_religion_manager().expect_religion_identifier(
			assign_variable_callback_pointer_opt(entry.religion)
		),
		"government", ZERO_OR_ONE, politics_manager.get_government_type_manager().expect_government_type_identifier(
			assign_variable_callback_pointer_opt(entry.government_type)
		),
		"plurality", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.plurality)),
		"nationalvalue", ZERO_OR_ONE, politics_manager.get_national_value_manager().expect_national_value_identifier(
			assign_variable_callback_pointer_opt(entry.national_value)
		),
		"civilized", ZERO_OR_ONE, expect_bool(assign_variable_callback(entry.civilised)),
		"prestige", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.prestige)),
		"ruling_party", ZERO_OR_ONE, [this, &entry](ast::NodeCPtr value) -> bool {
			country.expect_party_identifier(assign_variable_callback_pointer_opt(entry.ruling_party), true)(value);
			if (!entry.ruling_party.has_value()) {
				std::string_view def {};
				bool is_valid = true;
				
				expect_identifier(assign_variable_callback(def))(value);

				// if the party specified is invalid, replace with the first valid party
				for (const auto& party : country.get_parties()) {
					if (party.start_date <= entry.get_date() && entry.get_date() < party.end_date) {
						entry.ruling_party.emplace(&party);
						break;
					}
				}

				// if there's somehow no valid party, we use the first defined party even though it isn't valid for the entry
				if (!entry.ruling_party.has_value()) {
					is_valid = false;
					entry.ruling_party.emplace(&country.get_front_party());
				}
				
				spdlog::log_s(
					is_valid ? spdlog::level::warn : spdlog::level::err,
					"In {} history at entry date {}: ruling_party {} does NOT exist! Defaulting to first {}: {}",
					country,
					entry.get_date(),
					def,
					is_valid ? "valid party" : "INVALID party (no valid party was found)",
					*entry.ruling_party.value()
				);
			}
			return true;
		},
		"last_election", ZERO_OR_ONE, expect_date(assign_variable_callback(entry.last_election)),
		"upper_house", ZERO_OR_ONE, politics_manager.get_ideology_manager().expect_ideology_dictionary(
			[&entry](Ideology const& ideology, ast::NodeCPtr value) -> bool {
				return expect_fixed_point(map_callback(entry.upper_house_proportion_by_ideology, &ideology))(value);
			}
		),
		"oob", ZERO_OR_ONE, expect_identifier_or_string(
			[&definition_manager, &deployment_manager, &dataloader, &entry](std::string_view path) -> bool {
				Deployment const* deployment = nullptr;
				const bool ret = deployment_manager.load_oob_file(
					dataloader,
					definition_manager.get_map_definition(),
					definition_manager.get_military_manager(),
					path,
					deployment,
					false
				);
				if (deployment != nullptr) {
					entry.initial_oob = deployment;
				}
				return ret;
			}
		),
		"schools", ZERO_OR_ONE, technology_manager.expect_technology_school_identifier(
			assign_variable_callback_pointer_opt(entry.tech_school)
		),
		"foreign_investment", ZERO_OR_ONE,
			country_definition_manager.expect_country_definition_decimal_map(move_variable_callback(entry.foreign_investment)),
		"literacy", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.literacy)),
		"non_state_culture_literacy", ZERO_OR_ONE,
			expect_fixed_point(assign_variable_callback(entry.nonstate_culture_literacy)),
		"consciousness", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.consciousness)),
		"nonstate_consciousness", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.nonstate_consciousness)),
		"is_releasable_vassal", ZERO_OR_ONE, expect_bool(assign_variable_callback(entry.releasable_vassal)),
		"decision", ZERO_OR_MORE, decision_manager.expect_decision_identifier(set_callback_pointer(entry.decisions), true), // if a mod lists an invalid decision here (as some hpm-derived mods that remove hpm decisions do) vic2 just ignores it.
		"govt_flag", ZERO_OR_MORE, [&entry, &politics_manager](ast::NodeCPtr value) -> bool {
			GovernmentTypeManager const& government_type_manager = politics_manager.get_government_type_manager();
			GovernmentType const* government_type = nullptr;
			bool flag_expected = false;
			bool ret = expect_dictionary(
				[&entry, &government_type_manager, &government_type, &flag_expected](std::string_view id,
					ast::NodeCPtr node) -> bool {
					if (id == "government") {
						bool ret = true;
						if (flag_expected) {
							spdlog::error_s(
								"Government key found when expect flag type override for {} in history of {}",
								ovfmt::validate(government_type), entry.country
							);
							ret = false;
						}
						flag_expected = true;
						government_type = nullptr;
						ret &= government_type_manager.expect_government_type_identifier(
							assign_variable_callback_pointer(government_type)
						)(node);
						return ret;
					} else if (id == "flag") {
						if (flag_expected) {
							flag_expected = false;
							GovernmentType const* flag_override_government_type = nullptr;
							bool ret = government_type_manager.expect_government_type_identifier(
								assign_variable_callback_pointer(flag_override_government_type)
							)(node);
							/* If the first government type is null, the "government" section will have already output
							 * an error, so no need to output another one here. */
							if (government_type != nullptr && flag_override_government_type != nullptr) {
								ret &= map_callback(
									entry.flag_overrides_by_government_type, government_type
								)(flag_override_government_type);
							}
							return ret;
						} else {
							spdlog::error_s(
								"Flag key found when expecting government type for flag type override in history of {}",
								entry.country
							);
							return false;
						}
					} else {
						spdlog::error_s(
							"Invalid key {} in government flag overrides in history of {}",
							id, entry.country
						);
						return false;
					}
				}
			)(value);
			if (flag_expected) {
				spdlog::error_s(
					"Missing flag type override for government type {} in history of {}",
					ovfmt::validate(government_type), entry.country
				);
				ret = false;
			}
			return ret;
		},
		"colonial_points", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.colonial_points)),
		"set_country_flag", ZERO_OR_MORE, expect_identifier_or_string(_flag_callback(entry.country_flags, true)),
		"clr_country_flag", ZERO_OR_MORE, expect_identifier_or_string(_flag_callback(entry.country_flags, false)),
		"set_global_flag", ZERO_OR_MORE, expect_identifier_or_string(_flag_callback(entry.global_flags, true)),
		"clr_global_flag", ZERO_OR_MORE, expect_identifier_or_string(_flag_callback(entry.global_flags, false))
	)(root);
}

void CountryHistoryManager::reserve_more_country_histories(size_t size) {
	if (locked) {
		spdlog::error_s("Failed to reserve space for {} countries in CountryHistoryManager - already locked!", size);
	} else {
		reserve_more(country_histories, size);
	}
}

void CountryHistoryManager::lock_country_histories() {
	for (auto [country, history_map] : mutable_iterator(country_histories)) {
		history_map.sort_entries();
	}

	SPDLOG_INFO("Locked country history registry after registering {} items", country_histories.size());
	locked = true;
}

bool CountryHistoryManager::is_locked() const {
	return locked;
}

CountryHistoryMap const* CountryHistoryManager::get_country_history(CountryDefinition const& country) const {
	decltype(country_histories)::const_iterator country_registry = country_histories.find(&country);
	if (country_registry != country_histories.end()) {
		return &country_registry->second;
	} else {
		spdlog::error_s("Attempted to access history of country {} but none has been defined!", country);
		return nullptr;
	}
}

bool CountryHistoryManager::load_country_history_file(
	DefinitionManager& definition_manager, Dataloader const& dataloader, CountryDefinition const& country,
	decltype(CountryHistoryMap::ideology_keys) ideology_keys,
	decltype(CountryHistoryMap::government_type_keys) government_type_keys, ast::NodeCPtr root
) {
	if (locked) {
		spdlog::error_s("Attempted to load country history file for {} after country history registry was locked!", country);
		return false;
	}

	if (country.is_dynamic_tag()) {
		return true; /* as far as I can tell dynamic countries are hardcoded, broken, and unused */
	}

	decltype(country_histories)::iterator it = country_histories.find(&country);
	if (it == country_histories.end()) {
		const std::pair<decltype(country_histories)::iterator, bool> result =
			country_histories.emplace(&country, CountryHistoryMap { country, ideology_keys, government_type_keys });
		if (result.second) {
			it = result.first;
		} else {
			spdlog::error_s("Failed to create country history map for country {}", country);
			return false;
		}
	}
	CountryHistoryMap& country_history = it.value();

	return country_history._load_history_file(
		definition_manager, dataloader, definition_manager.get_military_manager().get_deployment_manager(), root
	);
}
