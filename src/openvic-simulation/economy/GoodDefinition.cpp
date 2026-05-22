#include "GoodDefinition.hpp"

using namespace OpenVic;

GoodCategory::GoodCategory(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

GoodDefinition::GoodDefinition(
	std::string_view new_identifier,
	colour_t new_colour,
	index_t new_index,
	GoodCategory const& new_category,
	fixed_point_t new_base_price,
	bool new_is_available_from_start,
	bool new_is_tradeable,
	bool new_is_money,
	bool new_counters_overseas_penalty
) : HasIdentifierAndColour { new_identifier, new_colour, false },
	HasIndex { new_index },
	category { new_category },
	base_price { new_base_price },
	is_available_from_start { new_is_available_from_start },
	is_tradeable { new_is_tradeable },
	is_money { new_is_money },
	counters_overseas_penalty { new_counters_overseas_penalty } {}
