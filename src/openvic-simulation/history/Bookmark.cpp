#include "Bookmark.hpp"

#include <type_safe/strong_typedef.hpp>

using namespace OpenVic;

Bookmark::Bookmark(
	index_t new_index,
	std::string_view new_name,
	std::string_view new_description,
	Date new_date,
	fvec2_t new_initial_camera_position
) : HasIdentifier { std::to_string(type_safe::get(new_index)) },
	HasIndex { new_index },
	name { new_name },
	description { new_description },
	date { new_date },
	initial_camera_position { new_initial_camera_position } {}
