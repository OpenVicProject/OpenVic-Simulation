#pragma once

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IndexedFlatMapMacro.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct Ideology;
	struct PartyPolicy;
	struct PartyPolicyGroup;

	struct CountryParty : HasIdentifierAndColour {
	private:
		const Date PROPERTY(start_date);
		const Date PROPERTY(end_date);
		Ideology const* PROPERTY(ideology); // Can be nullptr, shows up as "No Ideology" in game
		IndexedFlatMap_PROPERTY(PartyPolicyGroup, PartyPolicy const*, policies);

	public:
		CountryParty(
			std::string_view new_identifier,
			Date new_start_date,
			Date new_end_date,
			Ideology const* new_ideology,
			decltype(policies)&& new_policies
		);
		CountryParty(CountryParty&&) = default;
	};
}
#undef IndexedFlatMap_PROPERTY
#undef IndexedFlatMap_PROPERTY_ACCESS