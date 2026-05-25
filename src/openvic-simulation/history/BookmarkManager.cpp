#include "BookmarkManager.hpp"

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

bool BookmarkManager::add_bookmark(
	std::string_view name, std::string_view description, Date date, fvec2_t initial_camera_position
) {
	return bookmarks.emplace_item(
		name,
		Bookmark::index_t { bookmarks.size() }, name, description, date, initial_camera_position
	);
}

bool BookmarkManager::load_bookmark_file(fixed_point_t map_height, ovdl::v2script::ast::Node const* root) {
	const bool ret = expect_dictionary_reserve_length(
		bookmarks,
		[this, map_height](std::string_view key, ovdl::v2script::ast::Node const* value) -> bool {
			if (key != "bookmark") {
				spdlog::error_s("Invalid bookmark declaration {}", key);
				return false;
			}

			std::string_view name, description;
			Date date;
			fvec2_t initial_camera_position;

			bool ret = expect_dictionary_keys(
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(name)),
				"desc", ONE_EXACTLY, expect_string(assign_variable_callback(description)),
				"date", ONE_EXACTLY, expect_date(assign_variable_callback(date)),
				"cameraX", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(initial_camera_position.x)),
				"cameraY", ONE_EXACTLY,
					expect_fixed_point(flip_height_callback(assign_variable_callback(initial_camera_position.y), map_height))
			)(value);

			ret &= add_bookmark(name, description, date, initial_camera_position);
			return ret;
		}
	)(root);
	lock_bookmarks();

	return ret;
}

Date BookmarkManager::get_last_bookmark_date() const {
	Date ret {};
	for (Bookmark const& bookmark : get_bookmarks()) {
		if (bookmark.date > ret) {
			ret = bookmark.date;
		}
	}
	return ret;
}
