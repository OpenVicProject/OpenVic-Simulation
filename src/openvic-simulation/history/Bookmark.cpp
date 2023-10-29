#include "Bookmark.hpp"

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/dataloader/NodeTools.hpp"

#include "types/Date.hpp"
#include "types/IdentifierRegistry.hpp"
#include "utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Bookmark::Bookmark(
	size_t new_index, std::string_view new_name, std::string_view new_description, Date new_date, uint32_t new_initial_camera_x,
	uint32_t new_initial_camera_y
) : HasIdentifier { std::to_string(new_index) }, name { new_name }, description { new_description }, date { new_date },
	initial_camera_x { new_initial_camera_x }, initial_camera_y { new_initial_camera_y } {}

std::string_view Bookmark::get_name() const {
	return name;
}

std::string_view Bookmark::get_description() const {
	return description;
}

Date Bookmark::get_date() const {
	return date;
}

uint32_t Bookmark::get_initial_camera_x() const {
	return initial_camera_x;
}

uint32_t Bookmark::get_initial_camera_y() const {
	return initial_camera_y;
}

BookmarkManager::BookmarkManager() : bookmarks { "bookmarks" } {}

bool BookmarkManager::add_bookmark(
	std::string_view name, std::string_view description, Date date, uint32_t initial_camera_x, uint32_t initial_camera_y
) {
	return bookmarks.add_item({ bookmarks.size(), name, description, date, initial_camera_x, initial_camera_y });
}

bool BookmarkManager::load_bookmark_file(ast::NodeCPtr root) {
	const bool ret = expect_dictionary([this](std::string_view key, ast::NodeCPtr value) -> bool {
		if (key != "bookmark") {
			Logger::error("Invalid bookmark declaration ", key);
			return false;
		}

		std::string_view name, description;
		Date date;
		uint32_t initial_camera_x, initial_camera_y;

		bool ret = expect_dictionary_keys(
			"name", ONE_EXACTLY, expect_string(assign_variable_callback(name)),
			"desc", ONE_EXACTLY, expect_string(assign_variable_callback(description)),
			"date", ONE_EXACTLY, expect_date(assign_variable_callback(date)),
			"cameraX", ONE_EXACTLY, expect_uint(assign_variable_callback(initial_camera_x)),
			"cameraY", ONE_EXACTLY, expect_uint(assign_variable_callback(initial_camera_y))
		)(value);

		ret &= add_bookmark(name, description, date, initial_camera_x, initial_camera_y);
		return ret;
	})(root);
	lock_bookmarks();

	return ret;
}
