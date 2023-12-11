#include "CountryHistory.hpp"

#include "openvic-simulation/GameManager.hpp"
#include <string_view>

using namespace OpenVic;
using namespace OpenVic::NodeTools;

CountryHistoryEntry::CountryHistoryEntry(Country const& new_country, Date new_date)
	: HistoryEntry { new_date }, country { new_country } {}

CountryHistoryMap::CountryHistoryMap(Country const& new_country) : country { new_country } {}

std::unique_ptr<CountryHistoryEntry> CountryHistoryMap::_make_entry(Date date) const {
	return std::unique_ptr<CountryHistoryEntry> { new CountryHistoryEntry { country, date } };
}

bool CountryHistoryMap::_load_history_entry(
	GameManager const& game_manager, Dataloader const& dataloader, DeploymentManager& deployment_manager,
	CountryHistoryEntry& entry, ast::NodeCPtr root
) {
	PoliticsManager const& politics_manager = game_manager.get_politics_manager();
	IssueManager const& issue_manager = politics_manager.get_issue_manager();
	CultureManager const& culture_manager = game_manager.get_pop_manager().get_culture_manager();
	CountryManager const& country_manager = game_manager.get_country_manager();
	TechnologyManager const& technology_manager = game_manager.get_research_manager().get_technology_manager();
	InventionManager const& invention_manager = game_manager.get_research_manager().get_invention_manager();
	DecisionManager const& decision_manager = game_manager.get_decision_manager();

	return expect_dictionary_keys_and_default(
		[this, &game_manager, &dataloader, &deployment_manager, &issue_manager, 
			&technology_manager, &invention_manager, &country_manager, &entry](std::string_view key, ast::NodeCPtr value) -> bool {
			ReformGroup const* reform_group = issue_manager.get_reform_group_by_identifier(key);
			if (reform_group != nullptr) {
				return issue_manager.expect_reform_identifier([&entry, reform_group](Reform const& reform) -> bool {
					if (&reform.get_reform_group() != reform_group) {
						Logger::warning(
							"Listing ", reform.get_identifier(), " as belonging to the reform group ",
							reform_group->get_identifier(), " when it actually belongs to ",
							reform.get_reform_group().get_identifier(), " in history of ", entry.get_country().get_identifier()
						);
					}
					if (std::find(entry.reforms.begin(), entry.reforms.end(), &reform) != entry.reforms.end()) {
						Logger::error(
							"Redefinition of reform ", reform.get_identifier(), " in history of ",
							entry.get_country().get_identifier()
						);
						return false;
					}
					entry.reforms.push_back(&reform);
					return true;
				})(value);
			}

			Technology const* technology = technology_manager.get_technology_by_identifier(key);
			if (technology != nullptr) {
				bool flag;
				if (expect_int_bool(assign_variable_callback(flag))(value)) {
					return entry.technologies.emplace(technology, flag).second;
				} else return false;
			}

			Invention const* invention = invention_manager.get_invention_by_identifier(key);
			if (invention != nullptr) {
				bool flag;
				if (expect_bool(assign_variable_callback(flag))(value)) {
					return entry.inventions.emplace(invention, flag).second;
				} else return false;
			}
			
			return _load_history_sub_entry_callback(
				game_manager, dataloader, deployment_manager, entry.get_date(), value, key, value
			);
		},
		"capital", ZERO_OR_ONE,
			game_manager.get_map().expect_province_identifier(assign_variable_callback_pointer(entry.capital)),
		"primary_culture", ZERO_OR_ONE,
			culture_manager.expect_culture_identifier(assign_variable_callback_pointer(entry.primary_culture)),
		"culture", ZERO_OR_MORE, culture_manager.expect_culture_identifier(
			vector_callback_pointer(entry.accepted_cultures)
		),
		"religion", ZERO_OR_ONE, game_manager.get_pop_manager().get_religion_manager().expect_religion_identifier(
			assign_variable_callback_pointer(entry.religion)
		),
		"government", ZERO_OR_ONE, politics_manager.get_government_type_manager().expect_government_type_identifier(
			assign_variable_callback_pointer(entry.government_type)
		),
		"plurality", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.plurality)),
		"nationalvalue", ZERO_OR_ONE, politics_manager.get_national_value_manager().expect_national_value_identifier(
			assign_variable_callback_pointer(entry.national_value)
		),
		"civilized", ZERO_OR_ONE, expect_bool(assign_variable_callback(entry.civilised)),
		"prestige", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.prestige)),
		"ruling_party", ZERO_OR_ONE, country.expect_party_identifier(assign_variable_callback_pointer(entry.ruling_party)),
		"last_election", ZERO_OR_ONE, expect_date(assign_variable_callback(entry.last_election)),
		"upper_house", ZERO_OR_ONE, politics_manager.get_ideology_manager().expect_ideology_dictionary(
			[&entry](Ideology const& ideology, ast::NodeCPtr value) -> bool {
				return expect_fixed_point([&entry, &ideology](fixed_point_t val) -> bool {
					entry.upper_house[&ideology] = val;
					return true;
				})(value);
			}
		),
		"oob", ZERO_OR_ONE, expect_identifier_or_string(
			[&game_manager, &deployment_manager, &dataloader, &entry](std::string_view path) -> bool {
				Deployment const* deployment = nullptr;
				const bool ret = deployment_manager.load_oob_file(game_manager, dataloader, path, deployment, false);
				if (deployment != nullptr) {
					entry.inital_oob = deployment;
				}
				return ret;
			}
		),
		"schools", ZERO_OR_ONE, technology_manager.expect_technology_school_identifier(
			assign_variable_callback_pointer(entry.tech_school)
		),
		"foreign_investment", ZERO_OR_ONE, country_manager.expect_country_decimal_map(move_variable_callback(entry.foreign_investment)),
		"literacy", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.literacy)),
		"non_state_culture_literacy", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.nonstate_culture_literacy)),
		"consciousness", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.consciousness)),
		"nonstate_consciousness", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.nonstate_consciousness)),
		"is_releasable_vassal", ZERO_OR_ONE, expect_bool(assign_variable_callback(entry.releasable_vassal)),
		"decision", ZERO_OR_MORE, decision_manager.expect_decision_identifier([&entry](Decision const& decision) -> bool {
			return entry.decisions.emplace(&decision).second;
		}),
		"govt_flag", ZERO_OR_ONE, [&entry, &politics_manager](ast::NodeCPtr value) -> bool {
			GovernmentTypeManager const& government_type_manager = politics_manager.get_government_type_manager();
			GovernmentType const* government_type = nullptr;
			return expect_dictionary(
				[&entry, &government_type_manager, &government_type](std::string_view id, ast::NodeCPtr node) -> bool {
					bool ret = true;
					if (id == "government") {
						government_type = government_type_manager.get_government_type_by_identifier(id);
						ret &= government_type != nullptr;
					} else if (id == "flag") {
						std::string_view flag;
						ret &= expect_identifier_or_string(assign_variable_callback(flag))(node);
						ret &= entry.government_flags.emplace(government_type, flag).second;
						government_type = nullptr;
					}
					return ret;
				}
			)(value);
		},
		"colonial_points", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(entry.colonial_points)), 
		"set_country_flag", ZERO_OR_MORE, expect_identifier_or_string([&entry](std::string_view flag) -> bool {
			return entry.country_flags.emplace(flag).second;
		}),
		"set_global_flag", ZERO_OR_MORE, expect_identifier_or_string([&entry](std::string_view flag) -> bool {
			return entry.global_flags.emplace(flag).second;
		})
	)(root);
}

void CountryHistoryManager::lock_country_histories() {
	Logger::info("Locked country history registry after registering ", country_histories.size(), " items");
	locked = true;
}

bool CountryHistoryManager::is_locked() const {
	return locked;
}

CountryHistoryMap const* CountryHistoryManager::get_country_history(Country const* country) const {
	if (country == nullptr) {
		Logger::error("Attempted to access history of null country");
		return nullptr;
	}
	decltype(country_histories)::const_iterator country_registry = country_histories.find(country);
	if (country_registry != country_histories.end()) {
		return &country_registry->second;
	} else {
		Logger::error("Attempted to access history of country ", country->get_identifier(), " but none has been defined!");
		return nullptr;
	}
}

bool CountryHistoryManager::load_country_history_file(
	GameManager& game_manager, Dataloader const& dataloader, Country const& country, ast::NodeCPtr root
) {
	if (locked) {
		Logger::error(
			"Attempted to load country history file for ", country.get_identifier(),
			" after country history registry was locked!"
		);
		return false;
	}

	if (country.is_dynamic_tag()) {
		return true; /* as far as I can tell dynamic countries are hardcoded, broken, and unused */
	}

	decltype(country_histories)::iterator it = country_histories.find(&country);
	if (it == country_histories.end()) {
		const std::pair<decltype(country_histories)::iterator, bool> result =
			country_histories.emplace(&country, CountryHistoryMap { country });
		if (result.second) {
			it = result.first;
		} else {
			Logger::error("Failed to create country history map for country ", country.get_identifier());
			return false;
		}
	}
	CountryHistoryMap& country_history = it->second;

	return country_history._load_history_file(
		game_manager, dataloader, game_manager.get_military_manager().get_deployment_manager(), root
	);
}
