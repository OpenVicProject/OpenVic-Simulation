#include "Government.hpp"

#include <range/v3/algorithm/contains.hpp>

#include "openvic-simulation/politics/Ideology.hpp"

using namespace OpenVic;

GovernmentType::GovernmentType(
	index_t new_index,
	std::string_view new_identifier,
	memory::vector<std::reference_wrapper<const Ideology>>&& new_ideologies,
	bool new_holds_elections,
	bool new_can_appoint_ruling_party,
	Timespan new_term_duration,
	std::string_view new_flag_type_identifier
) : HasIndex { new_index },
	HasIdentifier { new_identifier },
	ideologies { std::move(new_ideologies) },
	holds_elections { new_holds_elections },
	can_appoint_ruling_party { new_can_appoint_ruling_party },
	term_duration { new_term_duration },
	flag_type_identifier { new_flag_type_identifier } {}

bool GovernmentType::is_ideology_compatible(Ideology const& ideology) const {
	return ranges::contains(
		ideologies,
		ideology
	);
}
