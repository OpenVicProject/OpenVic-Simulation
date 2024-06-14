#pragma once

#include <optional>
#include <vector>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/history/Period.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct DiplomaticHistoryManager;
	struct CountryDefinition;
	struct WargoalType;
	struct ProvinceDefinition;

	struct WarHistory {
		friend struct DiplomaticHistoryManager;

		struct added_wargoal_t {
			friend struct DiplomaticHistoryManager;

		private:
			Date PROPERTY_CUSTOM_PREFIX(added, get_date);
			CountryDefinition const* PROPERTY(actor);
			CountryDefinition const* PROPERTY(receiver);
			WargoalType const* PROPERTY(wargoal);

			// TODO - could these just be nullptr when unset rather than using optionals?
			std::optional<CountryDefinition const*> PROPERTY(third_party);
			std::optional<ProvinceDefinition const*> PROPERTY(target);

			added_wargoal_t(
				Date new_added, CountryDefinition const* new_actor, CountryDefinition const* new_receiver,
				WargoalType const* new_wargoal, std::optional<CountryDefinition const*> new_third_party,
				std::optional<ProvinceDefinition const*> new_target
			);
		};

		struct war_participant_t {
			friend struct DiplomaticHistoryManager;

		private:
			CountryDefinition const* PROPERTY(country);
			Period PROPERTY(period);

			war_participant_t(CountryDefinition const* new_country, Period period);
		};

	private:
		/* Edge cases where this is empty/undef for some reason,
		 * probably need to just generate war names like usual for that. */
		std::string PROPERTY(war_name);
		std::vector<war_participant_t> PROPERTY(attackers);
		std::vector<war_participant_t> PROPERTY(defenders);
		std::vector<added_wargoal_t> PROPERTY(wargoals);

		WarHistory(
			std::string_view new_war_name, std::vector<war_participant_t>&& new_attackers,
			std::vector<war_participant_t>&& new_defenders, std::vector<added_wargoal_t>&& new_wargoals
		);
	};

	struct AllianceHistory {
		friend struct DiplomaticHistoryManager;

	private:
		CountryDefinition const* PROPERTY(first);
		CountryDefinition const* PROPERTY(second);
		const Period PROPERTY(period);

		AllianceHistory(CountryDefinition const* new_first, CountryDefinition const* new_second, Period period);
	};

	struct ReparationsHistory {
		friend struct DiplomaticHistoryManager;

	private:
		CountryDefinition const* PROPERTY(receiver);
		CountryDefinition const* PROPERTY(sender);
		const Period PROPERTY(period);

		ReparationsHistory(CountryDefinition const* new_receiver, CountryDefinition const* new_sender, Period period);
	};

	struct SubjectHistory {
		friend struct DiplomaticHistoryManager;

		enum class type_t {
			VASSAL,
			UNION,
			SUBSTATE
		};

	private:
		CountryDefinition const* PROPERTY(overlord);
		CountryDefinition const* PROPERTY(subject);
		const type_t PROPERTY_CUSTOM_PREFIX(type, get_subject);
		const Period PROPERTY(period);

		SubjectHistory(
			CountryDefinition const* new_overlord, CountryDefinition const* new_subject, type_t new_type, Period period
		);
	};

	struct CountryDefinitionManager;
	struct DefinitionManager;

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
		/* Returns all wars that begin before date. NOTE: Some wargoals may be added or countries may join after date,
		 * should be checked for by functions that use get_wars() */
		std::vector<WarHistory const*> get_wars(Date date) const;

		bool load_diplomacy_history_file(CountryDefinitionManager const& country_definition_manager, ast::NodeCPtr root);
		bool load_war_history_file(DefinitionManager const& definition_manager, ast::NodeCPtr root);
	};
}
