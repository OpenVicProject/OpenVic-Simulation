#include "CountryHistory.hpp"

#include "openvic-simulation/GameManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

CountryHistory::CountryHistory(
	Culture const* new_primary_culture, std::vector<Culture const*>&& new_accepted_cultures, Religion const* new_religion,
	CountryParty const* new_ruling_party, Date new_last_election, decimal_map_t<Ideology const*>&& new_upper_house,
	Province const* new_capital, GovernmentType const* new_government_type, fixed_point_t new_plurality,
	NationalValue const* new_national_value, bool new_civilised, fixed_point_t new_prestige,
	std::vector<Reform const*>&& new_reforms, Deployment const* new_inital_oob
) : primary_culture { new_primary_culture }, accepted_cultures { std::move(new_accepted_cultures) },
	religion { new_religion }, ruling_party { new_ruling_party }, last_election { new_last_election },
	upper_house { std::move(new_upper_house) }, capital { new_capital }, government_type { new_government_type },
	plurality { new_plurality }, national_value { new_national_value }, civilised { new_civilised },
	prestige { new_prestige }, reforms { std::move(new_reforms) }, inital_oob { new_inital_oob } {}

Culture const* CountryHistory::get_primary_culture() const {
	return primary_culture;
}

std::vector<Culture const*> const& CountryHistory::get_accepted_cultures() const {
	return accepted_cultures;
}

Religion const* CountryHistory::get_religion() const {
	return religion;
}

CountryParty const* CountryHistory::get_ruling_party() const {
	return ruling_party;
}

Date CountryHistory::get_last_election() const {
	return last_election;
}

decimal_map_t<Ideology const*> const& CountryHistory::get_upper_house() const {
	return upper_house;
}

Province const* CountryHistory::get_capital() const {
	return capital;
}

GovernmentType const* CountryHistory::get_government_type() const {
	return government_type;
}

fixed_point_t CountryHistory::get_plurality() const {
	return plurality;
}

NationalValue const* CountryHistory::get_national_value() const {
	return national_value;
}

bool CountryHistory::is_civilised() const {
	return civilised;
}

fixed_point_t CountryHistory::get_prestige() const {
	return prestige;
}

std::vector<Reform const*> const& CountryHistory::get_reforms() const {
	return reforms;
}

Deployment const* CountryHistory::get_inital_oob() const {
	return inital_oob;
}

bool CountryHistoryManager::add_country_history_entry(
	Country const* country, Date date, Culture const* primary_culture, std::vector<Culture const*>&& accepted_cultures,
	Religion const* religion, CountryParty const* ruling_party, Date last_election,
	decimal_map_t<Ideology const*>&& upper_house, Province const* capital, GovernmentType const* government_type,
	fixed_point_t plurality, NationalValue const* national_value, bool civilised, fixed_point_t prestige,
	std::vector<Reform const*>&& reforms, std::optional<Deployment const*> initial_oob, bool updated_accepted_cultures,
	bool updated_upper_house, bool updated_reforms
) {
	if (locked) {
		Logger::error("Cannot add new history entry to country history registry: locked!");
		return false;
	}

	/* combine duplicate histories, priority to current (defined later) */
	country_history_map_t& country_registry = country_histories[country];
	const country_history_map_t::iterator existing_entry = country_registry.find(date);

	if (existing_entry != country_registry.end()) {
		if (primary_culture != nullptr) {
			existing_entry->second.primary_culture = primary_culture;
		}
		if (updated_accepted_cultures) {
			existing_entry->second.accepted_cultures = std::move(accepted_cultures);
		}
		if (religion != nullptr) {
			existing_entry->second.religion = religion;
		}
		if (ruling_party != nullptr) {
			existing_entry->second.ruling_party = ruling_party;
		}
		if (last_election != Date{}) {
			existing_entry->second.last_election = last_election;
		}
		if (updated_upper_house) {
			existing_entry->second.upper_house = std::move(upper_house);
		}
		if (capital != nullptr) {
			existing_entry->second.capital = capital;
		}
		if (government_type != nullptr) {
			existing_entry->second.government_type = government_type;
		}
		if (plurality >= 0) {
			existing_entry->second.plurality = plurality;
		}
		if (national_value != nullptr) {
			existing_entry->second.national_value = national_value;
		}
		if (civilised) {
			existing_entry->second.civilised = true;
		}
		if (prestige >= 0) {
			existing_entry->second.prestige = prestige;
		}
		if (updated_reforms) {
			existing_entry->second.reforms = std::move(reforms);
		}
		if (initial_oob) {
			existing_entry->second.inital_oob = *initial_oob;
		}
	} else {
		country_registry.emplace( date,
			CountryHistory {
				primary_culture, std::move(accepted_cultures), religion, ruling_party, last_election,
				std::move(upper_house), capital, government_type, plurality, national_value, civilised,
				prestige, std::move(reforms), std::move(*initial_oob)
			}
		);
	}
	return true;
}

void CountryHistoryManager::lock_country_histories() {
	for (const auto& entry : country_histories) {
		if (entry.second.size() == 0) {
			Logger::error(
				"Attempted to lock country histories - country ", entry.first->get_identifier(), " has no history entries!"
			);
		}
	}
	Logger::info("Locked country history registry after registering ", country_histories.size(), " items");
	locked = true;
}

bool CountryHistoryManager::is_locked() const {
	return locked;
}

CountryHistory const* CountryHistoryManager::get_country_history(Country const* country, Date entry) const {
	Date closest_entry;
	auto country_registry = country_histories.find(country);

	if (country_registry == country_histories.end()) {
		Logger::error("Attempted to access history of undefined country ", country->get_identifier());
		return nullptr;
	}

	for (const auto& current : country_registry->second) {
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

	auto entry_registry = country_registry->second.find(closest_entry);
	if (entry_registry != country_registry->second.end()) {
		return &entry_registry->second;
	}
	/* warned about lack of entries earlier, return nullptr */
	return nullptr;
}

inline CountryHistory const* CountryHistoryManager::get_country_history(Country const* country, Bookmark const* entry) const {
	return get_country_history(country, entry->get_date());
}

inline bool CountryHistoryManager::_load_country_history_entry(
	GameManager& game_manager, Dataloader const& dataloader, Country const& country, Date date, ast::NodeCPtr root
) {
	PoliticsManager const& politics_manager = game_manager.get_politics_manager();
	IssueManager const& issue_manager = politics_manager.get_issue_manager();
	CultureManager const& culture_manager = game_manager.get_pop_manager().get_culture_manager();

	Province const* capital = nullptr;
	Culture const* primary_culture = nullptr;
	Religion const* religion = nullptr;
	GovernmentType const* government_type = nullptr;
	NationalValue const* national_value = nullptr;
	CountryParty const* ruling_party = nullptr;
	std::vector<Culture const*> accepted_cultures {};
	std::vector<Reform const*> reforms {};
	decimal_map_t<Ideology const*> upper_house {};
	fixed_point_t plurality = -1, prestige = -1;
	bool civilised = false;
	Date last_election {};
	std::optional<Deployment const*> initial_oob;

	bool updated_accepted_cultures = false, updated_upper_house = false, updated_reforms = false;

	bool ret = expect_dictionary_keys_and_default(
		[this, &issue_manager, &reforms, &updated_reforms, &country](std::string_view key, ast::NodeCPtr value) -> bool {
			ReformGroup const* reform_group = issue_manager.get_reform_group_by_identifier(key);
			if (reform_group != nullptr) {
				updated_reforms = true;
				return issue_manager.expect_reform_identifier(
					[reform_group, &reforms, &country](Reform const& reform) -> bool {
						if (&reform.get_reform_group() != reform_group) {
							Logger::warning(
								"Listing ", reform.get_identifier(), " as belonging to the reform group ",
								reform_group->get_identifier(), " when it actually belongs to ",
								reform.get_reform_group().get_identifier()
							);
						}
						if (std::find(reforms.begin(), reforms.end(), &reform) != reforms.end()) {
							Logger::error(
								"Redefinition of reform ", reform.get_identifier(), " in history of ", country.get_identifier()
							);
							return false;
						}
						reforms.push_back(&reform);
						return true;
					}
				)(value);
			}
			// TODO: technologies & inventions
			return true;
		},
		/* we have to use a lambda, assign_variable_callback_pointer
		 * apparently doesn't play nice with const & non-const accessors */
		// TODO - fix this issue (cause by provinces having non-const accessors)
		"capital", ZERO_OR_ONE,
			game_manager.get_map().expect_province_identifier([&capital](Province const& province) -> bool {
				capital = &province;
				return true;
			}),
		"primary_culture", ZERO_OR_ONE,
			culture_manager.expect_culture_identifier(assign_variable_callback_pointer(primary_culture)),
		"culture", ZERO_OR_MORE,
			culture_manager.expect_culture_identifier(
				[&game_manager, &accepted_cultures, &updated_accepted_cultures](Culture const& culture) -> bool {
					updated_accepted_cultures = true;
					accepted_cultures.push_back(&culture);
					return true;
				}
			),
		"religion", ZERO_OR_ONE,
			game_manager.get_pop_manager().get_religion_manager().expect_religion_identifier(
				assign_variable_callback_pointer(religion)
			),
		"government", ZERO_OR_ONE,
			politics_manager.get_government_type_manager().expect_government_type_identifier(
				assign_variable_callback_pointer(government_type)
			),
		"plurality", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(plurality)),
		"nationalvalue", ZERO_OR_ONE,
			politics_manager.get_national_value_manager().expect_national_value_identifier(
				assign_variable_callback_pointer(national_value)
			),
		"civilized", ZERO_OR_ONE, expect_bool(assign_variable_callback(civilised)),
		"prestige", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(prestige)),
		"ruling_party", ZERO_OR_ONE, country.expect_party_identifier(assign_variable_callback_pointer(ruling_party)),
		"last_election", ZERO_OR_ONE, expect_date(assign_variable_callback(last_election)),
		"upper_house", ZERO_OR_ONE,
			politics_manager.get_ideology_manager().expect_ideology_dictionary(
				[&upper_house, &updated_upper_house](Ideology const& ideology, ast::NodeCPtr value) -> bool {
					return expect_fixed_point([&upper_house, &updated_upper_house, &ideology](fixed_point_t val) -> bool {
						if (val != 0) {
							upper_house[&ideology] += val;
							updated_upper_house = true;
						}
						return true;
					})(value);
				}
			),
		"oob", ZERO_OR_ONE, expect_identifier_or_string([&game_manager, &dataloader, &initial_oob](std::string_view path) -> bool {
			if(!initial_oob.has_value())
				initial_oob = decltype(initial_oob)::value_type {};
			return game_manager.get_military_manager().get_deployment_manager().load_oob_file(
				game_manager, dataloader, path, *initial_oob, false
			);
		}),
		"schools", ZERO_OR_ONE, success_callback, // TODO: technology school
		"foreign_investment", ZERO_OR_ONE, success_callback // TODO: foreign investment
	)(root);

	ret &= add_country_history_entry(
		&country, date, primary_culture,
		std::move(accepted_cultures), religion, ruling_party, last_election, std::move(upper_house), capital, government_type,
		plurality, national_value, civilised, prestige, std::move(reforms), std::move(initial_oob), updated_accepted_cultures,
		updated_upper_house, updated_reforms
	);
	return ret;
}

bool CountryHistoryManager::load_country_history_file(
	GameManager& game_manager, Dataloader const& dataloader, Country const& country, ast::NodeCPtr root
) {
	if (country.is_dynamic_tag()) {
		return true; /* as far as I can tell dynamic countries are hardcoded, broken, and unused */
	}

	bool ret = _load_country_history_entry(
		game_manager, dataloader, country, game_manager.get_define_manager().get_start_date(), root
	);

	ret &= expect_dictionary([this, &game_manager, &dataloader, &country](std::string_view key, ast::NodeCPtr value) -> bool {
		bool is_date = false;
		Date entry = Date::from_string(key, &is_date, true);
		if (!is_date) {
			return true;
		}

		Date end_date = game_manager.get_define_manager().get_end_date();
		if (entry > end_date) {
			Logger::error(
				"History entry ", entry.to_string(), " of country ", country.get_identifier(),
				" defined after defined end date ", end_date.to_string()
			);
			return false;
		}

		return _load_country_history_entry(game_manager, dataloader, country, entry, value);
	})(root);

	return ret;
}
