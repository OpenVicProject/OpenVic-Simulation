#pragma once

#include <optional>
#include <type_traits>

#include "openvic-simulation/history/Period.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct CountryDefinition;
	struct ProvinceDefinition;
	struct WargoalType;
	
	struct WarHistory {
		friend struct DiplomaticHistoryManager;

		struct added_wargoal_t {		
		public:
			const Date date_added;
			CountryDefinition const& actor;
			CountryDefinition const& receiver;
			WargoalType const& wargoal;

			// TODO - could these just be nullptr when unset rather than using optionals?
			const std::optional<CountryDefinition const*> third_party;
			const std::optional<ProvinceDefinition const*> target;
			
			constexpr added_wargoal_t(
				const Date new_date_added,
				CountryDefinition const& new_actor,
				CountryDefinition const& new_receiver,
				WargoalType const& new_wargoal,
				std::optional<CountryDefinition const*> new_third_party,
				std::optional<ProvinceDefinition const*> new_target
			) : date_added { new_date_added },
				actor { new_actor },
				receiver { new_receiver },
				wargoal { new_wargoal },
				third_party { new_third_party },
				target { new_target } {}
		};
		static_assert(std::is_trivially_move_constructible_v<added_wargoal_t>);

		struct war_participant_t {
		private:
			friend struct DiplomaticHistoryManager;
			Period PROPERTY(period);

		public:
			CountryDefinition const& country;
			
			constexpr war_participant_t(CountryDefinition const& new_country, const Period new_period)
				: country { new_country }, period { new_period } {}
		};
		static_assert(std::is_trivially_move_constructible_v<war_participant_t>);

		WarHistory(
			std::string_view new_war_name, memory::vector<war_participant_t>&& new_attackers,
			memory::vector<war_participant_t>&& new_defenders, memory::vector<added_wargoal_t>&& new_wargoals
		);

	private:
		/* Edge cases where this is empty/undef for some reason,
		 * probably need to just generate war names like usual for that. */
		memory::string PROPERTY(war_name);
		memory::vector<war_participant_t> SPAN_PROPERTY(attackers);
		memory::vector<war_participant_t> SPAN_PROPERTY(defenders);
		memory::vector<added_wargoal_t> SPAN_PROPERTY(wargoals);
	};
}