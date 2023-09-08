#include "IdentifierRegistry.hpp"

#include <cassert>

using namespace OpenVic;

HasIdentifier::HasIdentifier(const std::string_view new_identifier)
	: identifier { new_identifier } {
	assert(!identifier.empty());
}

std::string const& HasIdentifier::get_identifier() const {
	return identifier;
}

HasColour::HasColour(colour_t const new_colour, bool can_be_null) : colour(new_colour) {
	assert((can_be_null || colour != NULL_COLOUR) && colour <= MAX_COLOUR_RGB);
}

colour_t HasColour::get_colour() const { return colour; }

std::string HasColour::colour_to_hex_string() const {
	return OpenVic::colour_to_hex_string(colour);
}

HasIdentifierAndColour::HasIdentifierAndColour(const std::string_view new_identifier,
	const colour_t new_colour, bool can_be_null)
	: HasIdentifier { new_identifier },
	  HasColour { new_colour, can_be_null } {}

distribution_t::value_type OpenVic::get_largest_item(distribution_t const& dist) {
	const distribution_t::const_iterator result = std::max_element(dist.begin(), dist.end(),
		[](distribution_t::value_type a, distribution_t::value_type b) -> bool {
			return a.second < b.second;
		});
	return result != dist.end() ? *result : distribution_t::value_type { nullptr, -1.0f };
}
