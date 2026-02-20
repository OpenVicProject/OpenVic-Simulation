#pragma once

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"

namespace OpenVic {
	struct Ideology;
	struct PartyPolicy;
	struct PartyPolicyGroup;

	struct CountryParty : HasIdentifierAndColour {
	private:
		OV_IFLATMAP_PROPERTY(PartyPolicyGroup, PartyPolicy const*, policies);

	public:
		const Date start_date;
		const Date end_date;
		Ideology const* const ideology; // Can be nullptr, shows up as "No Ideology" in game

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