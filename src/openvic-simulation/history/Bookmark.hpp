#pragma once

#include <cstdint>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct BookmarkManager;

	struct Bookmark : HasIdentifier {
		friend struct BookmarkManager;

	private:
		const std::string name;
		const std::string description;
		const Date date;
		const uint32_t initial_camera_x;
		const uint32_t initial_camera_y;

		Bookmark(
			size_t new_index, std::string_view new_name, std::string_view new_description, Date new_date,
			uint32_t new_initial_camera_x, uint32_t new_initial_camera_y
		);

	public:
		Bookmark(Bookmark&&) = default;

		std::string_view get_name() const;
		std::string_view get_description() const;
		Date get_date() const;
		uint32_t get_initial_camera_x() const;
		uint32_t get_initial_camera_y() const;
	};

	struct BookmarkManager {
	private:
		IdentifierRegistry<Bookmark> bookmarks;

	public:
		BookmarkManager();

		bool add_bookmark(
			std::string_view name, std::string_view description, Date date, uint32_t initial_camera_x,
			uint32_t initial_camera_y
		);
		IDENTIFIER_REGISTRY_ACCESSORS(bookmark)

		bool load_bookmark_file(ast::NodeCPtr root);
	};
}
