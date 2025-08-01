#include "CountryParty.hpp"

#include "openvic-simulation/politics/Ideology.hpp"

using namespace OpenVic;

CountryParty::CountryParty(
	std::string_view new_identifier, Date new_start_date, Date new_end_date, Ideology const* new_ideology,
	decltype(policies)&& new_policies
) : HasIdentifierAndColour {
		new_identifier,
		new_ideology != nullptr ? new_ideology->get_colour() : Ideology::NO_IDEOLOGY_COLOUR,
		false
	},
	start_date { new_start_date },
	end_date { new_end_date },
	ideology { new_ideology },
	policies { std::move(new_policies) } {}