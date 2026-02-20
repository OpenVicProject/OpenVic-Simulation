#pragma once

#include <type_traits>

#include "openvic-simulation/history/Period.hpp"

namespace OpenVic {
	struct CountryDefinition;
	struct ReparationsHistory {
	public:
		CountryDefinition const& receiver;
		CountryDefinition const& sender;
		const Period period;

		constexpr ReparationsHistory(
			CountryDefinition const& new_receiver,
			CountryDefinition const& new_sender,
			const Period new_period
		) : receiver { new_receiver }, sender { new_sender }, period { new_period } {}
	};
	static_assert(std::is_trivially_move_constructible_v<ReparationsHistory>);
}