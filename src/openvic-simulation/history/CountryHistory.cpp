#include "CountryHistory.hpp"

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/DefinitionManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

CountryHistoryEntry::CountryHistoryEntry(
	CountryDefinition const& new_country, Date new_date, decltype(upper_house)::keys_type const& ideology_keys,
	decltype(government_flag_overrides)::keys_type const& government_type_keys
) : HistoryEntry { new_date }, country { new_country }, upper_house { &ideology_keys },
	government_flag_overrides { &government_type_keys } {}

CountryHistoryMap::CountryHistoryMap(
	CountryDefinition const& new_country, decltype(ideology_keys) new_ideology_keys,
	decltype(government_type_keys) new_government_type_keys
) : country { new_country }, ideology_keys { new_ideology_keys }, government_type_keys { new_government_type_keys } {}

std::unique_ptr<CountryHistoryEntry> CountryHistoryMap::_make_entry(Date date) const {
	return std::unique_ptr<CountryHistoryEntry> {
		new CountryHistoryEntry { country, date, ideology_keys, government_type_keys }
	};
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
				Logger::warning(
					"Duplicate attempt to ", add ? "add" : "remove", " accepted culture ", culture.get_identifier(),
					" ", add ? "to" : "from", " country history of ", entry.get_country()
				);
				return true;
			} else {
				// Opposite culture instruction exists
				entry.accepted_cultures.erase(it);
				Logger::warning(
					"Attempted to ", add ? "add" : "remove", " accepted culture ", culture.get_identifier(),
					" ", add ? "to" : "from", " country history of ", entry.get_country(),
					" after previously ", add ? "removing" : "adding", " it"
				);
				return true;
			}
		};
	};

	return expect_dictionary_keys_and_default(
		[this, &definition_manager, &dataloader, &deployment_manager, &issue_manager, &technology_manager, &invention_manager,
			&country_definition_manager, &entry](std::string_view key, ast::NodeCPtr value) -> bool {
			ReformGroup const* reform_group = issue_manager.get_reform_group_by_identifier(key);
			if (reform_group != nullptr) {
				return issue_manager.expect_reform_identifier([&entry, reform_group](Reform const& reform) -> bool {
					if (&reform.get_reform_group() != reform_group) {
						Logger::warning(
							"Listing ", reform.get_identifier(), " as belonging to the reform group ", reform_group,
							" when it actually belongs to ", reform.get_reform_group(), " in history of ", entry.get_country()
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
								Logger::warning(
									"Technology ", technology->get_identifier(),
									" is applied multiple times in history of country ", entry.get_country().get_identifier()
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
				definition_manager, dataloader, deployment_manager, entry.get_date(), value, key, value
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
		"ruling_party", ZERO_OR_ONE, country.expect_party_identifier(assign_variable_callback_pointer_opt(entry.ruling_party)),
		"last_election", ZERO_OR_ONE, expect_date(assign_variable_callback(entry.last_election)),
		"upper_house", ZERO_OR_ONE, politics_manager.get_ideology_manager().expect_ideology_dictionary(
			[&entry](Ideology const& ideology, ast::NodeCPtr value) -> bool {
				return expect_fixed_point(map_callback(entry.upper_house, &ideology))(value);
			}
		),
		"oob", ZERO_OR_ONE, expect_identifier_or_string(
			[&definition_manager, &deployment_manager, &dataloader, &entry](std::string_view path) -> bool {
				Deployment const* deployment = nullptr;
				const bool ret = deployment_manager.load_oob_file(definition_manager, dataloader, path, deployment, false);
				if (deployment != nullptr) {
					entry.inital_oob = deployment;
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
		"decision", ZERO_OR_MORE, decision_manager.expect_decision_identifier(set_callback_pointer(entry.decisions)),
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
							Logger::error(
								"Government key found when expect flag type override for ", government_type,
								" in history of ", entry.get_country()
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
									entry.government_flag_overrides, government_type)(flag_override_government_type
								);
							}
							return ret;
						} else {
							Logger::error(
								"Flag key found when expecting government type for flag type override in history of ",
								entry.get_country()
							);
							return false;
						}
					} else {
						Logger::error(
							"Invalid key ", id, " in government flag overrides in history of ", entry.get_country()
						);
						return false;
					}
				}
			)(value);
			if (flag_expected) {
				Logger::error(
					"Missing flag type override for government type ", government_type, " in history of ", entry.get_country()
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
		Logger::error("Failed to reserve space for ", size, " countries in CountryHistoryManager - already locked!");
	} else {
		reserve_more(country_histories, size);
	}
}

void CountryHistoryManager::lock_country_histories() {
	for (auto [country, history_map] : mutable_iterator(country_histories)) {
		history_map.sort_entries();
	}

	Logger::info("Locked country history registry after registering ", country_histories.size(), " items");
	locked = true;
}

bool CountryHistoryManager::is_locked() const {
	return locked;
}

CountryHistoryMap const* CountryHistoryManager::get_country_history(CountryDefinition const* country) const {
	if (country == nullptr) {
		Logger::error("Attempted to access history of null country");
		return nullptr;
	}
	decltype(country_histories)::const_iterator country_registry = country_histories.find(country);
	if (country_registry != country_histories.end()) {
		return &country_registry->second;
	} else {
		Logger::error("Attempted to access history of country ", country, " but none has been defined!");
		return nullptr;
	}
}

bool CountryHistoryManager::load_country_history_file(
	DefinitionManager& definition_manager, Dataloader const& dataloader, CountryDefinition const& country,
	decltype(CountryHistoryMap::ideology_keys) ideology_keys,
	decltype(CountryHistoryMap::government_type_keys) government_type_keys, ast::NodeCPtr root
) {
	if (locked) {
		Logger::error("Attempted to load country history file for ", country, " after country history registry was locked!");
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
			Logger::error("Failed to create country history map for country ", country);
			return false;
		}
	}
	CountryHistoryMap& country_history = it.value();

	return country_history._load_history_file(
		definition_manager, dataloader, definition_manager.get_military_manager().get_deployment_manager(), root
	);
}
