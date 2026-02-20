#pragma once

#include <type_traits>

#include "openvic-simulation/history/Period.hpp"

namespace OpenVic {
	struct CountryDefinition;
	struct SubjectHistory {
	public:
		enum struct type_t : uint8_t {
			VASSAL,
			UNION,
			SUBSTATE
		};
		CountryDefinition const& overlord;
		CountryDefinition const& subject;
		const type_t subject_type;
		const Period period;

		constexpr SubjectHistory(
			CountryDefinition const& new_overlord,
			CountryDefinition const& new_subject,
			const type_t new_subject_type,
			const Period new_period
		) : overlord { new_overlord }, subject { new_subject }, subject_type { new_subject_type }, period { new_period } {}
	};
	static_assert(std::is_trivially_move_constructible_v<SubjectHistory>);
}