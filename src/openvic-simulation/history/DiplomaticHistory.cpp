#include "DiplomaticHistory.hpp"

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

WarHistory::added_wargoal_t::added_wargoal_t(
	Date new_added,
	CountryDefinition const* new_actor,
	CountryDefinition const* new_receiver,
	WargoalType const* new_wargoal,
	std::optional<CountryDefinition const*> new_third_party,
	std::optional<ProvinceDefinition const*> new_target
) : added { new_added }, actor { new_actor }, receiver { new_receiver }, wargoal { new_wargoal },
	third_party { new_third_party }, target { new_target } {}

WarHistory::war_participant_t::war_participant_t(
	CountryDefinition const* new_country,
	Period new_period
) : country { new_country }, period { new_period } {}

WarHistory::WarHistory(
	std::string_view new_war_name,
	memory::vector<war_participant_t>&& new_attackers,
	memory::vector<war_participant_t>&& new_defenders,
	memory::vector<added_wargoal_t>&& new_wargoals
) : war_name { new_war_name }, attackers { std::move(new_attackers) }, defenders { std::move(new_defenders) },
	wargoals { std::move(new_wargoals) } {}

AllianceHistory::AllianceHistory(
	CountryDefinition const* new_first,
	CountryDefinition const* new_second,
	Period new_period
) : first { new_first }, second { new_second }, period { new_period } {}

ReparationsHistory::ReparationsHistory(
	CountryDefinition const* new_receiver,
	CountryDefinition const* new_sender,
	Period new_period
) : receiver { new_receiver }, sender { new_sender }, period { new_period } {}

SubjectHistory::SubjectHistory(
	CountryDefinition const* new_overlord,
	CountryDefinition const* new_subject,
	type_t new_subject_type,
	Period new_period
) : overlord { new_overlord }, subject { new_subject }, subject_type { new_subject_type }, period { new_period } {}

void DiplomaticHistoryManager::reserve_more_wars(size_t size) {
	if (locked) {
		spdlog::error_s("Failed to reserve space for {} wars in DiplomaticHistoryManager - already locked!", size);
	} else {
		reserve_more(wars, size);
	}
}

void DiplomaticHistoryManager::lock_diplomatic_history() {
	SPDLOG_INFO(
		"Locked diplomacy history registry after registering {} items", alliances.size() + subjects.size() + wars.size()
	);
	locked = true;
}

bool DiplomaticHistoryManager::is_locked() const {
	return locked;
}

memory::vector<AllianceHistory const*> DiplomaticHistoryManager::get_alliances(Date date) const {
	memory::vector<AllianceHistory const*> ret {};
	for (auto const& alliance : alliances) {
		if (alliance.period.is_date_in_period(date)) {
			ret.push_back(&alliance);
		}
	}
	return ret;
}

memory::vector<ReparationsHistory const*> DiplomaticHistoryManager::get_reparations(Date date) const {
	memory::vector<ReparationsHistory const*> ret {};
	for (auto const& reparation : reparations) {
		if (reparation.period.is_date_in_period(date)) {
			ret.push_back(&reparation);
		}
	}
	return ret;
}

memory::vector<SubjectHistory const*> DiplomaticHistoryManager::get_subjects(Date date) const {
	memory::vector<SubjectHistory const*> ret {};
	for (auto const& subject : subjects) {
		if (subject.period.is_date_in_period(date)) {
			ret.push_back(&subject);
		}
	}
	return ret;
}

memory::vector<WarHistory const*> DiplomaticHistoryManager::get_wars(Date date) const {
	memory::vector<WarHistory const*> ret;
	for (auto const& war : wars) {
		Date start {};
		for (auto const& wargoal : war.wargoals) {
			if (wargoal.added < start) {
				start = wargoal.added;
			}
		}
		if (start >= date) {
			ret.push_back(&war);
		}
	}
	return ret;
}

bool DiplomaticHistoryManager::load_diplomacy_history_file(
	CountryDefinitionManager const& country_definition_manager, ast::NodeCPtr root
) {
	// Default vanilla alliances is 2, 54 is the max I've seen in mods
	// Eliminates reallocations at the cost of memory usage
	alliances.reserve(16);
	// Default vanilla subjects is 42, 152 is the max I've seen in mods
	// Eliminates reallocations at the cost of memory usage
	subjects.reserve(64);
	// Default vanilla reparations is 0, yet to see a mod use it
	// Eliminates reallocations at the cost of memory usage
	reparations.reserve(1);

	return expect_dictionary_keys(
		"alliance", ZERO_OR_MORE, [this, &country_definition_manager](ast::NodeCPtr node) -> bool {
			CountryDefinition const* first = nullptr;
			CountryDefinition const* second = nullptr;
			Date start {};
			std::optional<Date> end {};

			bool ret = expect_dictionary_keys(
				"first", ONE_EXACTLY, country_definition_manager.expect_country_definition_identifier_or_string(
					assign_variable_callback_pointer(first)
				),
				"second", ONE_EXACTLY, country_definition_manager.expect_country_definition_identifier_or_string(
					assign_variable_callback_pointer(second)
				),
				"start_date", ONE_EXACTLY, expect_date_identifier_or_string(assign_variable_callback(start)),
				"end_date", ZERO_OR_ONE, expect_date_identifier_or_string(assign_variable_callback(end))
			)(node);

			alliances.emplace_back(first, second, Period { start, end });
			return ret;
		},
		"vassal", ZERO_OR_MORE, [this, &country_definition_manager](ast::NodeCPtr node) -> bool {
			CountryDefinition const* overlord = nullptr;
			CountryDefinition const* subject = nullptr;
			Date start {};
			std::optional<Date> end {};

			bool ret = expect_dictionary_keys(
				"first", ONE_EXACTLY, country_definition_manager.expect_country_definition_identifier_or_string(
					assign_variable_callback_pointer(overlord)
				),
				"second", ONE_EXACTLY, country_definition_manager.expect_country_definition_identifier_or_string(
					assign_variable_callback_pointer(subject)
				),
				"start_date", ONE_EXACTLY, expect_date_identifier_or_string(assign_variable_callback(start)),
				"end_date", ZERO_OR_ONE, expect_date_identifier_or_string(assign_variable_callback(end))
			)(node);

			subjects.emplace_back(overlord, subject, SubjectHistory::type_t::VASSAL, Period { start, end });
			return ret;
		},
		"union", ZERO_OR_MORE, [this, &country_definition_manager](ast::NodeCPtr node) -> bool {
			CountryDefinition const* overlord = nullptr;
			CountryDefinition const* subject = nullptr;
			Date start {};
			std::optional<Date> end {};

			bool ret = expect_dictionary_keys(
				"first", ONE_EXACTLY, country_definition_manager.expect_country_definition_identifier_or_string(
					assign_variable_callback_pointer(overlord)
				),
				"second", ONE_EXACTLY, country_definition_manager.expect_country_definition_identifier_or_string(
					assign_variable_callback_pointer(subject)
				),
				"start_date", ONE_EXACTLY, expect_date_identifier_or_string(assign_variable_callback(start)),
				"end_date", ZERO_OR_ONE, expect_date_identifier_or_string(assign_variable_callback(end))
			)(node);

			subjects.emplace_back(overlord, subject, SubjectHistory::type_t::UNION, Period { start, end });
			return ret;
		},
		"substate", ZERO_OR_MORE, [this, &country_definition_manager](ast::NodeCPtr node) -> bool {
			CountryDefinition const* overlord = nullptr;
			CountryDefinition const* subject = nullptr;
			Date start {};
			std::optional<Date> end {};

			bool ret = expect_dictionary_keys(
				"first", ONE_EXACTLY, country_definition_manager.expect_country_definition_identifier_or_string(
					assign_variable_callback_pointer(overlord)
				),
				"second", ONE_EXACTLY, country_definition_manager.expect_country_definition_identifier_or_string(
					assign_variable_callback_pointer(subject)
				),
				"start_date", ONE_EXACTLY, expect_date_identifier_or_string(assign_variable_callback(start)),
				"end_date", ZERO_OR_ONE, expect_date_identifier_or_string(assign_variable_callback(end))
			)(node);

			subjects.emplace_back(overlord, subject, SubjectHistory::type_t::SUBSTATE, Period { start, end });
			return ret;
		},
		"reparations", ZERO_OR_MORE, [this, &country_definition_manager](ast::NodeCPtr node) -> bool {
			CountryDefinition const* receiver = nullptr;
			CountryDefinition const* sender = nullptr;
			Date start {};
			std::optional<Date> end {};

			bool ret = expect_dictionary_keys(
				"first", ONE_EXACTLY, country_definition_manager.expect_country_definition_identifier_or_string(
					assign_variable_callback_pointer(receiver)
				),
				"second", ONE_EXACTLY, country_definition_manager.expect_country_definition_identifier_or_string(
					assign_variable_callback_pointer(sender)
				),
				"start_date", ONE_EXACTLY, expect_date_identifier_or_string(assign_variable_callback(start)),
				"end_date", ZERO_OR_ONE, expect_date_identifier_or_string(assign_variable_callback(end))
			)(node);

			reparations.emplace_back(receiver, sender, Period { start, end });
			return ret;
		}
	)(root);
}

bool DiplomaticHistoryManager::load_war_history_file(DefinitionManager const& definition_manager, ast::NodeCPtr root) {
	std::string_view name {};

	// Ensures that if ever multithreaded, only one vector is used per thread
	// Else acts like static
	thread_local memory::vector<WarHistory::war_participant_t> attackers;
	// Default max vanilla attackers is 1, 1 is the max I've seen in mods
	// Eliminates reallocations
	attackers.reserve(1);

	thread_local memory::vector<WarHistory::war_participant_t> defenders;
	// Default max vanilla defenders is 1, 1 is the max I've seen in mods
	// Eliminates reallocations
	defenders.reserve(1);

	thread_local memory::vector<WarHistory::added_wargoal_t> wargoals;
	// Default max vanilla wargoals is 2, 2 is the max I've seen in mods
	// Eliminates reallocations
	wargoals.reserve(1);

	Date current_date {};


	bool ret = expect_dictionary_keys_and_default(
		[&definition_manager, &current_date, &name](
			std::string_view key, ast::NodeCPtr node
		) -> bool {
			bool ret = expect_date_str(assign_variable_callback(current_date))(key);

			ret &= expect_dictionary_keys(
				"add_attacker", ZERO_OR_MORE,
					definition_manager.get_country_definition_manager().expect_country_definition_identifier(
						[&current_date, &name](CountryDefinition const& country) -> bool {
							for (WarHistory::war_participant_t const& attacker : attackers) {
								CountryDefinition const* const attacker_country = attacker.get_country();
								if (attacker_country && *attacker_country == country) {
									spdlog::error_s(
										"In history of war {} at date {}: Attempted to add attacking country {} which is already present!",
										name, current_date, *attacker_country
									);
									return false;
								}
							}

							attackers.emplace_back(&country, Period { current_date, {} });
							return true;
						}
					),
				"add_defender", ZERO_OR_MORE,
					definition_manager.get_country_definition_manager().expect_country_definition_identifier(
						[&current_date, &name](CountryDefinition const& country) -> bool {
							for (WarHistory::war_participant_t const& defender : defenders) {
								CountryDefinition const* const defender_country = defender.get_country();
								if (defender_country && *defender_country == country) {
									spdlog::error_s(
										"In history of war {} at date {}: Attempted to add defending country {} which is already present!",
										name, current_date, *defender_country
									);
									return false;
								}
							}

							defenders.emplace_back(&country, Period { current_date, {} });
							return true;
						}
					),
				"rem_attacker", ZERO_OR_MORE,
					definition_manager.get_country_definition_manager().expect_country_definition_identifier(
						[&current_date, &name](CountryDefinition const& country) -> bool {
							WarHistory::war_participant_t* participant_to_remove = nullptr;

							for (WarHistory::war_participant_t& attacker : attackers) {
								CountryDefinition const* const attacker_country = attacker.get_country();
								if (attacker_country && *attacker_country == country) {
									participant_to_remove = &attacker;
									break;
								}
							}

							if (participant_to_remove == nullptr) {
								spdlog::warn_s(
									"In history of war {} at date {}: Attempted to remove attacking country {} which was not present!",
									name, current_date, country
								);
								return true;
							}

							return participant_to_remove->period.try_set_end(current_date);
						}
					),
				"rem_defender", ZERO_OR_MORE,
					definition_manager.get_country_definition_manager().expect_country_definition_identifier(
						[&current_date, &name](CountryDefinition const& country) -> bool {
							WarHistory::war_participant_t* participant_to_remove = nullptr;

							for (WarHistory::war_participant_t& defender : defenders) {
								CountryDefinition const* const defender_country = defender.get_country();
								if (defender_country && *defender_country == country) {
									participant_to_remove = &defender;
									break;
								}
							}

							if (participant_to_remove == nullptr) {
								spdlog::warn_s(
									"In history of war {} at date {}: Attempted to remove defending country {} which was not present!",
									name, current_date, country
								);
								return true;
							}

							return participant_to_remove->period.try_set_end(current_date);
						}
					),
				"war_goal", ZERO_OR_MORE, [&definition_manager, &current_date](ast::NodeCPtr value) -> bool {
					CountryDefinition const* actor = nullptr;
					CountryDefinition const* receiver = nullptr;
					WargoalType const* type = nullptr;
					std::optional<CountryDefinition const*> third_party {};
					std::optional<ProvinceDefinition const*> target {};

					bool ret = expect_dictionary_keys(
						"actor", ONE_EXACTLY,
							definition_manager.get_country_definition_manager().expect_country_definition_identifier(
								assign_variable_callback_pointer(actor)
							),
						"receiver", ONE_EXACTLY,
							definition_manager.get_country_definition_manager().expect_country_definition_identifier(
								assign_variable_callback_pointer(receiver)
							),
						"casus_belli", ONE_EXACTLY, definition_manager.get_military_manager().get_wargoal_type_manager()
							.expect_wargoal_type_identifier(assign_variable_callback_pointer(type)),
						"country", ZERO_OR_ONE,
							definition_manager.get_country_definition_manager().expect_country_definition_identifier(
								assign_variable_callback_pointer_opt(third_party)
							),
						"state_province_id", ZERO_OR_ONE,
							definition_manager.get_map_definition().expect_province_definition_identifier(
								assign_variable_callback_pointer_opt(target)
							)
					)(value);

					wargoals.emplace_back(current_date, actor, receiver, type, third_party, target);
					return ret;
				}
			)(node);

			return ret;
		},
		"name", ZERO_OR_ONE, expect_string(assign_variable_callback(name))
	)(root);

	wars.emplace_back(name, std::move(attackers), std::move(defenders), std::move(wargoals));

	return ret;
}
