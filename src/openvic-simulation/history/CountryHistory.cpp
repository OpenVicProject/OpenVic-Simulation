#include "CountryHistory.hpp"

#include "openvic-simulation/GameManager.hpp"

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

	return expect_dictionary_keys_and_default(
		[this, &game_manager, &dataloader, &deployment_manager, &issue_manager, &entry](
			std::string_view key, ast::NodeCPtr value) -> bool {
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
			// TODO: technologies & inventions
			return _load_history_sub_entry_callback(
				game_manager, dataloader, deployment_manager, entry.get_date(), value, key, value, key_value_success_callback
			);
		},
		/* we have to use a lambda, assign_variable_callback_pointer
		 * apparently doesn't play nice with const & non-const accessors */
		// TODO - fix this issue (cause by provinces having non-const accessors)
		"capital", ZERO_OR_ONE,
			game_manager.get_map().expect_province_identifier(assign_variable_callback_pointer(entry.capital)),
		"primary_culture", ZERO_OR_ONE,
			culture_manager.expect_culture_identifier(assign_variable_callback_pointer(entry.primary_culture)),
		"culture", ZERO_OR_MORE, culture_manager.expect_culture_identifier(
			[&entry](Culture const& culture) -> bool {
				entry.accepted_cultures.push_back(&culture);
				return true;
			}
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
		"schools", ZERO_OR_ONE, success_callback, // TODO: technology school
		"foreign_investment", ZERO_OR_ONE, success_callback // TODO: foreign investment
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
