#pragma once

#include <type_traits>

#include "openvic-simulation/history/Period.hpp"

namespace OpenVic {
	struct CountryDefinition;
	struct AllianceHistory {
	public:
		CountryDefinition const& first;
		CountryDefinition const& second;
		const Period period;

		constexpr AllianceHistory(
			CountryDefinition const& new_first,
			CountryDefinition const& new_second,
			const Period new_period
		) : first { new_first }, second { new_second }, period { new_period } {}
	};
	static_assert(std::is_trivially_move_constructible_v<AllianceHistory>);
}