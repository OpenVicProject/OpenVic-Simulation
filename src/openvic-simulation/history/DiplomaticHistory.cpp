#include "DiplomaticHistory.hpp"

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/history/DiplomaticHistory.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

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

memory::vector<WarHistory const*> DiplomaticHistoryManager::get_wars(Date date) const {
	memory::vector<WarHistory const*> ret;
	for (auto const& war : wars) {
		Date start {};
		for (auto const& wargoal : war.wargoals) {
			if (wargoal.date_added < start) {
				start = wargoal.date_added;
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

			if (ret) {
				alliances.emplace_back(*first, *second, Period { start, end });
			}
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

			if (ret) {
				subjects.emplace_back(*overlord, *subject, SubjectHistory::type_t::VASSAL, Period { start, end });
			}
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

			if (ret) {
				subjects.emplace_back(*overlord, *subject, SubjectHistory::type_t::UNION, Period { start, end });
			}
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

			if (ret) {
				subjects.emplace_back(*overlord, *subject, SubjectHistory::type_t::SUBSTATE, Period { start, end });
			}
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

			if (ret) {
				reparations.emplace_back(*receiver, *sender, Period { start, end });
			}
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
								CountryDefinition const& attacker_country = attacker.country;
								if (attacker_country == country) {
									spdlog::error_s(
										"In history of war {} at date {}: Attempted to add attacking country {} which is already present!",
										name, current_date, attacker_country
									);
									return false;
								}
							}

							attackers.emplace_back(country, Period { current_date, {} });
							return true;
						}
					),
				"add_defender", ZERO_OR_MORE,
					definition_manager.get_country_definition_manager().expect_country_definition_identifier(
						[&current_date, &name](CountryDefinition const& country) -> bool {
							for (WarHistory::war_participant_t const& defender : defenders) {
								CountryDefinition const& defender_country = defender.country;
								if (defender_country == country) {
									spdlog::error_s(
										"In history of war {} at date {}: Attempted to add defending country {} which is already present!",
										name, current_date, defender_country
									);
									return false;
								}
							}

							defenders.emplace_back(country, Period { current_date, {} });
							return true;
						}
					),
				"rem_attacker", ZERO_OR_MORE,
					definition_manager.get_country_definition_manager().expect_country_definition_identifier(
						[&current_date, &name](CountryDefinition const& country) -> bool {
							WarHistory::war_participant_t* participant_to_remove = nullptr;

							for (WarHistory::war_participant_t& attacker : attackers) {
								CountryDefinition const& attacker_country = attacker.country;
								if (attacker_country == country) {
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
								CountryDefinition const& defender_country = defender.country;
								if (defender_country == country) {
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

					if (ret) {
						wargoals.emplace_back(current_date, *actor, *receiver, *type, third_party, target);
					}
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
