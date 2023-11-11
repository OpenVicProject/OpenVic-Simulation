#pragma once

#include <bitset>
#include <vector>
#include <map>
#include <optional>

#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/military/Wargoal.hpp"
#include "openvic-simulation/map/Province.hpp"

namespace OpenVic {
	struct DiplomaticHistoryManager;

	struct WarHistory {
		friend struct DiplomaticHistoryManager;

		struct added_wargoal_t {
			friend struct DiplomaticHistoryManager;

		private:
			Date added;
			Country const* actor;
			Country const* receiver;
			WargoalType const* wargoal;
			std::optional<Country const*> third_party;
			std::optional<Province const*> target;

			added_wargoal_t(Date new_added, Country const* new_actor, Country const* new_receiver, WargoalType const* new_wargoal, std::optional<Country const*> new_third_party, std::optional<Province const*> new_target);

		public:
			Date get_date_added() const;
			Country const* get_actor() const;
			Country const* get_receiver() const;
			WargoalType const* get_wargoal_type() const;
			std::optional<Country const*> const& get_third_party() const;
			std::optional<Province const*> const& get_target() const;
		};

		struct war_participant_t {
			friend struct DiplomaticHistoryManager;

		private:
			Country const* country;
			Date joined;
			std::optional<Date> exited;

			war_participant_t(Country const* new_country, Date new_joined, std::optional<Date> new_exited);
		
		public:
			Country const* get_country() const;
			Date get_date_joined() const;
			std::optional<Date> get_date_exited() const;
		};

	private:
		std::string war_name; // edge cases where this is empty/undef for some reason, probably need to just generate war names like usual for that.
		std::vector<war_participant_t> attackers;
		std::vector<war_participant_t> defenders;
		std::vector<added_wargoal_t> wargoals;

		WarHistory(std::string_view new_war_name, std::vector<war_participant_t>&& new_attackers, std::vector<war_participant_t>&& new_defenders, std::vector<added_wargoal_t>&& new_wargoals);

	public:
		std::string_view get_war_name() const;
		std::vector<war_participant_t> const& get_attackers() const;
		std::vector<war_participant_t> const& get_defenders() const;
		std::vector<added_wargoal_t> const& get_wargoals() const;
	};

	struct AllianceHistory {
		friend struct DiplomaticHistoryManager;

	private:
		Country const* first;
		Country const* second;
		const Date start;
		const Date end;

		AllianceHistory(Country const* new_first, Country const* new_second, const Date new_start, const Date new_end);

	public:
		Country const* get_first() const;
		Country const* get_second() const;
	};

	struct SubjectHistory {
		friend struct DiplomaticHistoryManager;

		enum class type_t {
			VASSAL,
			UNION,
			SUBSTATE
		};

	private:
		Country const* overlord;
		Country const* subject;
		const type_t type;
		const Date start;
		const Date end;

		SubjectHistory(Country const* new_overlord, Country const* new_subject, const type_t new_type, const Date new_start, const Date new_end);

	public:
		Country const* get_overlord() const;
		Country const* get_subject() const;
		const type_t get_subject_type() const; 
	};

	struct DiplomaticHistoryManager {
	private:
		std::vector<AllianceHistory> alliances;
		std::vector<SubjectHistory> subjects;
		std::vector<WarHistory> wars;
		bool locked = false;

	public:
		DiplomaticHistoryManager() {}

		void lock_diplomatic_history();
		bool is_locked() const;

		std::vector<AllianceHistory const*> get_alliances(Date date) const;
		std::vector<SubjectHistory const*> get_subjects(Date date) const;
		/* Returns all wars that begin before date. NOTE: Some wargoals may be added or countries may join after date, should be checked for by functions that use get_wars() */
		std::vector<WarHistory const*> get_wars(Date date) const;

		bool load_diplomacy_history_file(GameManager& game_manager, ast::NodeCPtr root);
		bool load_war_history_file(GameManager& game_manager, ast::NodeCPtr root);
	};
} // namespace OpenVic