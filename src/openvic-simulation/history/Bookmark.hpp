#pragma once

#include <fmt/base.h>

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/types/Vector.hpp"

namespace OpenVic {
	struct Bookmark : HasIdentifier, HasIndex<Bookmark, bookmark_index_t> {

	private:
		memory::string PROPERTY(name);
		memory::string PROPERTY(description);

	public:
		const Date date;
		const fvec2_t initial_camera_position;

		Bookmark(
			index_t new_index,
			std::string_view new_name,
			std::string_view new_description,
			Date new_date,
			fvec2_t new_initial_camera_position
		);
		Bookmark(Bookmark&&) = default;
	};
}

template<std::same_as<OpenVic::Bookmark> T, typename char_type>
struct fmt::formatter<T, char_type> : formatter<string_view> {
	using Bookmark = OpenVic::Bookmark;
	static_assert(!std::same_as<T, T>, "Bookmark formatting is ambiguous, use get_identifier() or get_name()");
	format_context::iterator format(Bookmark const&, format_context& ctx) const {
		return ctx.out();
	}
};