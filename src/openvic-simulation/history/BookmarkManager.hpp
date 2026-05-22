#pragma once

#include <fmt/base.h>

#include "openvic-simulation/history/Bookmark.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct BookmarkManager {
	private:
		IdentifierRegistry<Bookmark> IDENTIFIER_REGISTRY(bookmark);

	public:
		bool add_bookmark(
			std::string_view name, std::string_view description, Date date, fvec2_t initial_camera_position
		);
		bool load_bookmark_file(fixed_point_t map_height, ovdl::v2script::ast::Node const* root);

		Date get_last_bookmark_date() const;
	};
}
