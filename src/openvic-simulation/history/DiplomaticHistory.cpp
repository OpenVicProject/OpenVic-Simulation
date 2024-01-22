#include "DiplomaticHistory.hpp"

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/history/Period.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

WarHistory::added_wargoal_t::added_wargoal_t(
	Date new_added,
	Country const* new_actor,
	Country const* new_receiver,
	WargoalType const* new_wargoal,
	std::optional<Country const*> new_third_party,
	std::optional<Province const*> new_target
) : added { new_added }, actor { new_actor }, receiver { new_receiver }, wargoal { new_wargoal }, third_party { new_third_party }, target { new_target } {}

WarHistory::war_participant_t::war_participant_t(
	Country const* new_country,
	const Period new_period
) : country { new_country }, period { new_period } {}

WarHistory::WarHistory(
	std::string_view new_war_name,
	std::vector<war_participant_t>&& new_attackers,
	std::vector<war_participant_t>&& new_defenders,
	std::vector<added_wargoal_t>&& new_wargoals
) : war_name { new_war_name }, attackers { std::move(new_attackers) }, defenders { std::move(new_defenders) }, wargoals { std::move(new_wargoals) } {}

AllianceHistory::AllianceHistory(
	Country const* new_first,
	Country const* new_second,
	const Period new_period
) : first { new_first }, second { new_second }, period {new_period} {}

ReparationsHistory::ReparationsHistory(
	Country const* new_receiver,
	Country const* new_sender,
	const Period new_period
) : receiver { new_receiver }, sender { new_sender }, period {new_period} {}

SubjectHistory::SubjectHistory(
	Country const* new_overlord,
	Country const* new_subject,
	const type_t new_type,
	const Period new_period
) : overlord { new_overlord }, subject { new_subject }, type { new_type }, period {new_period} {}

void DiplomaticHistoryManager::reserve_more_wars(size_t size) {
	if (locked) {
		Logger::error("Failed to reserve space for ", size, " wars in DiplomaticHistoryManager - already locked!");
	} else {
		reserve_more(wars, size);
	}
}

void DiplomaticHistoryManager::lock_diplomatic_history() {
	Logger::info("Locked diplomacy history registry after registering ", alliances.size() + subjects.size() + wars.size(), " items");
	locked = true;
}

bool DiplomaticHistoryManager::is_locked() const {
	return locked;
}

std::vector<AllianceHistory const*> DiplomaticHistoryManager::get_alliances(Date date) const {
	std::vector<AllianceHistory const*> ret {};
	for (const auto& alliance : alliances) {
		if (alliance.period.is_date_in_period(date)) {
			ret.push_back(&alliance);
		}
	}
	return ret;
}

std::vector<ReparationsHistory const*> DiplomaticHistoryManager::get_reparations(Date date) const {
	std::vector<ReparationsHistory const*> ret {};
	for (const auto& reparation : reparations) {
		if (reparation.period.is_date_in_period(date)) {
			ret.push_back(&reparation);
		}
	}
	return ret;
}

std::vector<SubjectHistory const*> DiplomaticHistoryManager::get_subjects(Date date) const {
	std::vector<SubjectHistory const*> ret {};
	for (const auto& subject : subjects) {
		if (subject.period.is_date_in_period(date)) {
			ret.push_back(&subject);
		}
	}
	return ret;
}

std::vector<WarHistory const*> DiplomaticHistoryManager::get_wars(Date date) const {
	std::vector<WarHistory const*> ret;
	for (const auto& war : wars) {
		Date start {};
		for (const auto& wargoal : war.wargoals) {
			if (wargoal.added < start) start = wargoal.added;
		}
		if (start >= date) ret.push_back(&war);
	}
	return ret;
}

bool DiplomaticHistoryManager::load_diplomacy_history_file(CountryManager const& country_manager, ast::NodeCPtr root) {
	return expect_dictionary_keys(
		"alliance", ZERO_OR_MORE, [this, &country_manager](ast::NodeCPtr node) -> bool {
			Country const* first = nullptr;
			Country const* second = nullptr;
			Date start {};
			std::optional<Date> end {};

			bool ret = expect_dictionary_keys(
				"first", ONE_EXACTLY, expect_identifier_or_string(country_manager.expect_country_str(assign_variable_callback_pointer(first))),
				"second", ONE_EXACTLY, expect_identifier_or_string(country_manager.expect_country_str(assign_variable_callback_pointer(second))),
				"start_date", ONE_EXACTLY, expect_identifier_or_string(expect_date_str(assign_variable_callback(start))),
				"end_date", ZERO_OR_ONE, expect_identifier_or_string(expect_date_str(assign_variable_callback(end)))
			)(node);

			alliances.push_back({ first, second, { start, end } });
			return ret;
		},
		"vassal", ZERO_OR_MORE, [this, &country_manager](ast::NodeCPtr node) -> bool {
			Country const* overlord = nullptr;
			Country const* subject = nullptr;
			Date start {};
			std::optional<Date> end {};

			bool ret = expect_dictionary_keys(
				"first", ONE_EXACTLY, expect_identifier_or_string(country_manager.expect_country_str(assign_variable_callback_pointer(overlord))),
				"second", ONE_EXACTLY, expect_identifier_or_string(country_manager.expect_country_str(assign_variable_callback_pointer(subject))),
				"start_date", ONE_EXACTLY, expect_identifier_or_string(expect_date_str(assign_variable_callback(start))),
				"end_date", ZERO_OR_ONE, expect_identifier_or_string(expect_date_str(assign_variable_callback(end)))
			)(node);

			subjects.push_back({ overlord, subject, SubjectHistory::type_t::VASSAL, { start, end } });
			return ret;
		},
		"union", ZERO_OR_MORE, [this, &country_manager](ast::NodeCPtr node) -> bool {
			Country const* overlord = nullptr;
			Country const* subject = nullptr;
			Date start {};
			std::optional<Date> end {};

			bool ret = expect_dictionary_keys(
				"first", ONE_EXACTLY, country_manager.expect_country_identifier(assign_variable_callback_pointer(overlord)),
				"second", ONE_EXACTLY, country_manager.expect_country_identifier(assign_variable_callback_pointer(subject)),
				"start_date", ONE_EXACTLY, expect_date(assign_variable_callback(start)),
				"end_date", ZERO_OR_ONE, expect_date(assign_variable_callback(end))
			)(node);

			subjects.push_back({ overlord, subject, SubjectHistory::type_t::UNION, { start, end } });
			return ret;
		},
		"substate", ZERO_OR_MORE, [this, &country_manager](ast::NodeCPtr node) -> bool {
			Country const* overlord = nullptr;
			Country const* subject = nullptr;
			Date start {};
			std::optional<Date> end {};

			bool ret = expect_dictionary_keys(
				"first", ONE_EXACTLY, country_manager.expect_country_identifier(assign_variable_callback_pointer(overlord)),
				"second", ONE_EXACTLY, country_manager.expect_country_identifier(assign_variable_callback_pointer(subject)),
				"start_date", ONE_EXACTLY, expect_date(assign_variable_callback(start)),
				"end_date", ZERO_OR_ONE, expect_date(assign_variable_callback(end))
			)(node);

			subjects.push_back({ overlord, subject, SubjectHistory::type_t::SUBSTATE, { start, end } });
			return ret;
		},
		"reparations", ZERO_OR_MORE, [this, &country_manager](ast::NodeCPtr node) -> bool {
			Country const* receiver = nullptr;
			Country const* sender = nullptr;
			Date start {};
			std::optional<Date> end {};

			bool ret = expect_dictionary_keys(
				"first", ONE_EXACTLY, country_manager.expect_country_identifier(assign_variable_callback_pointer(receiver)),
				"second", ONE_EXACTLY, country_manager.expect_country_identifier(assign_variable_callback_pointer(sender)),
				"start_date", ONE_EXACTLY, expect_date(assign_variable_callback(start)),
				"end_date", ZERO_OR_ONE, expect_date(assign_variable_callback(end))
			)(node);

			reparations.push_back({ receiver, sender, { start, end } });
			return ret;
		}
	)(root);
}

bool DiplomaticHistoryManager::load_war_history_file(GameManager const& game_manager, ast::NodeCPtr root) {
	std::string_view name {};
	std::vector<WarHistory::war_participant_t> attackers {};
	std::vector<WarHistory::war_participant_t> defenders {};
	std::vector<WarHistory::added_wargoal_t> wargoals {};
	Date current_date {};

	bool ret = expect_dictionary_keys_and_default(
		[&game_manager, &attackers, &defenders, &wargoals, &current_date, &name](std::string_view key, ast::NodeCPtr node) -> bool {
			bool ret = expect_date_str(assign_variable_callback(current_date))(key);
			ret &= expect_dictionary_keys(
				"add_attacker", ZERO_OR_MORE, game_manager.get_country_manager().expect_country_identifier([&attackers, &current_date, &name](Country const& country) -> bool {
					for (const auto& attacker : attackers) {
						if (attacker.get_country() == &country) {
							Logger::error("In history of war ", name, " at date ", current_date.to_string(), ": Attempted to add attacking country ", attacker.get_country()->get_identifier(), " which is already present!");
							return false;
						}
					}

					attackers.push_back({ &country, { current_date, {} } });
					return true;
				}),
				"add_defender", ZERO_OR_MORE, game_manager.get_country_manager().expect_country_identifier([&defenders, &current_date, &name](Country const& country) -> bool {
					for (const auto& defender : defenders) {
						if (defender.get_country() == &country) {
							Logger::error("In history of war ", name, " at date ", current_date.to_string(), ": Attempted to add defending country ", defender.get_country()->get_identifier(), " which is already present!");
							return false;
						}
					}

					defenders.push_back({ &country, { current_date, {} } });
					return true;
				}),
				"rem_attacker", ZERO_OR_MORE, game_manager.get_country_manager().expect_country_identifier([&attackers, &current_date, &name](Country const& country) -> bool {
					WarHistory::war_participant_t* participant_to_remove = nullptr;

					for (auto& attacker : attackers) {
						if (attacker.country == &country) {
							participant_to_remove = &attacker;
							break;
						}
					}

					if (participant_to_remove == nullptr) {
						Logger::warning("In history of war ", name, " at date ", current_date.to_string(), ": Attempted to remove attacking country ", country.get_identifier(), " which was not present!");
						return true;
					}

					return participant_to_remove->period.try_set_end(current_date);
				}),
				"rem_defender", ZERO_OR_MORE, game_manager.get_country_manager().expect_country_identifier([&defenders, &current_date, &name](Country const& country) -> bool {
					WarHistory::war_participant_t* participant_to_remove = nullptr;

					for (auto& defender : defenders) {
						if (defender.country == &country) {
							participant_to_remove = &defender;
							break;
						}
					}

					if (participant_to_remove == nullptr) {
						Logger::warning("In history of war ", name, " at date ", current_date.to_string(), ": Attempted to remove attacking country ", country.get_identifier(), " which was not present!");
						return true;
					}

					return participant_to_remove->period.try_set_end(current_date);
				}),
				"war_goal", ZERO_OR_MORE, [&game_manager, &wargoals, &current_date](ast::NodeCPtr value) -> bool {
					Country const* actor = nullptr;
					Country const* receiver = nullptr;
					WargoalType const* type = nullptr;
					std::optional<Country const*> third_party {};
					std::optional<Province const*> target {};

					bool ret = expect_dictionary_keys(
						"actor", ONE_EXACTLY, game_manager.get_country_manager().expect_country_identifier(assign_variable_callback_pointer(actor)),
						"receiver", ONE_EXACTLY, game_manager.get_country_manager().expect_country_identifier(assign_variable_callback_pointer(receiver)),
						"casus_belli", ONE_EXACTLY, game_manager.get_military_manager().get_wargoal_type_manager().expect_wargoal_type_identifier(assign_variable_callback_pointer(type)),
						"country", ZERO_OR_ONE, game_manager.get_country_manager().expect_country_identifier(assign_variable_callback_pointer(*third_party)),
						"state_province_id", ZERO_OR_ONE, game_manager.get_map().expect_province_identifier(assign_variable_callback_pointer(*target))
					)(value);
					wargoals.push_back({ current_date, actor, receiver, type, third_party, target });
					return ret;
				}
			)(node);
			return ret;
		},
		"name", ZERO_OR_ONE, expect_string(assign_variable_callback(name))
	)(root);

	wars.push_back({ name, std::move(attackers), std::move(defenders), std::move(wargoals) });
	return ret;
}