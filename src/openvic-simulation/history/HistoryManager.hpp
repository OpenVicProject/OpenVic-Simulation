#pragma once

#include "openvic-simulation/history/Bookmark.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct HistoryManager {
	private:
		BookmarkManager bookmark_manager;

	public:
		REF_GETTERS(bookmark_manager)

		inline bool load_bookmark_file(ast::NodeCPtr root) {
			return bookmark_manager.load_bookmark_file(root);
		}
	};
}
