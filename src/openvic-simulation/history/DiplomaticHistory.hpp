#pragma once

#include <vector>
#include <optional>

#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/military/Wargoal.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/history/Period.hpp"

namespace OpenVic {
	struct DiplomaticHistoryManager;

	struct WarHistory {
		friend struct DiplomaticHistoryManager;

		struct added_wargoal_t {
			friend struct DiplomaticHistoryManager;

		private:
			Date PROPERTY_CUSTOM_PREFIX(added, get_date);
			Country const* PROPERTY(actor);
			Country const* PROPERTY(receiver);
			WargoalType const* PROPERTY(wargoal);
			std::optional<Country const*> PROPERTY(third_party);
			std::optional<Province const*> PROPERTY(target);

			added_wargoal_t(Date new_added, Country const* new_actor, Country const* new_receiver, WargoalType const* new_wargoal, std::optional<Country const*> new_third_party, std::optional<Province const*> new_target);
		};

		struct war_participant_t {
			friend struct DiplomaticHistoryManager;

		private:
			Country const* PROPERTY(country);
			Period PROPERTY(period);

			war_participant_t(Country const* new_country, const Period period);
		};

	private:
		std::string PROPERTY(war_name); // edge cases where this is empty/undef for some reason, probably need to just generate war names like usual for that.
		std::vector<war_participant_t> PROPERTY(attackers);
		std::vector<war_participant_t> PROPERTY(defenders);
		std::vector<added_wargoal_t> PROPERTY(wargoals);

		WarHistory(std::string_view new_war_name, std::vector<war_participant_t>&& new_attackers, std::vector<war_participant_t>&& new_defenders, std::vector<added_wargoal_t>&& new_wargoals);
	};

	struct AllianceHistory {
		friend struct DiplomaticHistoryManager;

	private:
		Country const* PROPERTY(first);
		Country const* PROPERTY(second);
		const Period PROPERTY(period);

		AllianceHistory(Country const* new_first, Country const* new_second, const Period period);
	};

	struct ReparationsHistory {
		friend struct DiplomaticHistoryManager;

	private:
		Country const* PROPERTY(receiver);
		Country const* PROPERTY(sender);
		const Period PROPERTY(period);

		ReparationsHistory(Country const* new_receiver, Country const* new_sender, const Period period);
	};

	struct SubjectHistory {
		friend struct DiplomaticHistoryManager;

		enum class type_t {
			VASSAL,
			UNION,
			SUBSTATE
		};

	private:
		Country const* PROPERTY(overlord);
		Country const* PROPERTY(subject);
		const type_t PROPERTY_CUSTOM_PREFIX(type, get_subject);
		const Period PROPERTY(period);

		SubjectHistory(Country const* new_overlord, Country const* new_subject, const type_t new_type, const Period period);
	};

	struct DiplomaticHistoryManager {
	private:
		std::vector<AllianceHistory> alliances;
		std::vector<ReparationsHistory> reparations;
		std::vector<SubjectHistory> subjects;
		std::vector<WarHistory> wars;
		bool locked = false;

	public:
		DiplomaticHistoryManager() {}

		void reserve_more_wars(size_t size);
		void lock_diplomatic_history();
		bool is_locked() const;

		std::vector<AllianceHistory const*> get_alliances(Date date) const;
		std::vector<ReparationsHistory const*> get_reparations(Date date) const;
		std::vector<SubjectHistory const*> get_subjects(Date date) const;
		/* Returns all wars that begin before date. NOTE: Some wargoals may be added or countries may join after date, should be checked for by functions that use get_wars() */
		std::vector<WarHistory const*> get_wars(Date date) const;

		bool load_diplomacy_history_file(CountryManager const& country_manager, ast::NodeCPtr root);
		bool load_war_history_file(GameManager const& game_manager, ast::NodeCPtr root);
	};
} // namespace OpenVic