#pragma once

#include <cstdint>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct BookmarkManager;

	struct Bookmark : HasIdentifier {
		friend struct BookmarkManager;

	private:
		const std::string PROPERTY(name);
		const std::string PROPERTY(description);
		const Date PROPERTY(date);
		const uint32_t PROPERTY(initial_camera_x);
		const uint32_t PROPERTY(initial_camera_y);

		Bookmark(
			size_t new_index, std::string_view new_name, std::string_view new_description, Date new_date,
			uint32_t new_initial_camera_x, uint32_t new_initial_camera_y
		);

	public:
		Bookmark(Bookmark&&) = default;
	};

	struct BookmarkManager {
	private:
		IdentifierRegistry<Bookmark> IDENTIFIER_REGISTRY(bookmark);

	public:
		bool add_bookmark(
			std::string_view name, std::string_view description, Date date, uint32_t initial_camera_x,
			uint32_t initial_camera_y
		);
		bool load_bookmark_file(ast::NodeCPtr root);
	};
}
