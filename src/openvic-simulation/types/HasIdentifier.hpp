#pragma once

#include <algorithm>
#include <cassert>
#include <string_view>
#include <ostream>
#include <type_traits>

#include <fmt/base.h>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	constexpr bool valid_basic_identifier_char(char c) {
		return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || c == '_';
	}
	constexpr bool valid_basic_identifier(std::string_view identifier) {
		return std::all_of(identifier.begin(), identifier.end(), valid_basic_identifier_char);
	}
	constexpr std::string_view extract_basic_identifier_prefix(std::string_view identifier) {
		size_t len = 0;
		while (len < identifier.size() && valid_basic_identifier_char(identifier[len])) {
			++len;
		}
		return { identifier.data(), len };
	}

	/*
	 * Base class for objects with a non-empty string identifier. Uniquely named instances of a type derived from this class
	 * can be entered into an IdentifierRegistry instance.
	 */
	class HasIdentifier {
		/* Not const so it can be moved rather than needing to be copied. */
		memory::string PROPERTY(identifier);

	protected:
		HasIdentifier(std::string_view new_identifier): identifier { new_identifier } {
			assert(!identifier.empty());
		}
		HasIdentifier(HasIdentifier const&) = default;

	public:
		HasIdentifier(HasIdentifier&&) = default;
		HasIdentifier& operator=(HasIdentifier const&) = delete;
		HasIdentifier& operator=(HasIdentifier&&) = delete;
	};

	inline std::ostream& operator<<(std::ostream& stream, HasIdentifier const& obj) {
		return stream << obj.get_identifier();
	}
	inline std::ostream& operator<<(std::ostream& stream, HasIdentifier const* obj) {
		return obj != nullptr ? stream << *obj : stream << "<NULL>";
	}

	template<typename T>
	concept HasGetIdentifier = requires(T const& t) {
		{ t.get_identifier() } -> std::same_as<std::string_view>;
	};

	/*
	 * Base class for objects with associated colour information.
	 */
	template<IsColour ColourT>
	class _HasColour {
		const ColourT PROPERTY(colour);

	protected:
		_HasColour(ColourT new_colour, bool cannot_be_null): colour { new_colour } {
			assert(!cannot_be_null || !colour.is_null());
		}
		_HasColour(_HasColour const&) = default;

	public:
		_HasColour(_HasColour&&) = default;
		_HasColour& operator=(_HasColour const&) = delete;
		_HasColour& operator=(_HasColour&&) = delete;
	};

	using HasColour = _HasColour<colour_t>;
	using HasAlphaColour = _HasColour<colour_argb_t>;

	template<typename T>
	concept HasGetColour = requires(T const& t) {
		{ t.get_colour() } -> IsColour;
	};

	/*
	 * Base class for objects with a unique string identifier and associated colour information.
	 */
	template<IsColour ColourT>
	class _HasIdentifierAndColour : public HasIdentifier, public _HasColour<ColourT> {
	protected:
		_HasIdentifierAndColour(std::string_view new_identifier, ColourT new_colour, bool cannot_be_null)
			: HasIdentifier { new_identifier }, _HasColour<ColourT> { new_colour, cannot_be_null } {}
		_HasIdentifierAndColour(_HasIdentifierAndColour const&) = default;

	public:
		_HasIdentifierAndColour(_HasIdentifierAndColour&&) = default;
		_HasIdentifierAndColour& operator=(_HasIdentifierAndColour const&) = delete;
		_HasIdentifierAndColour& operator=(_HasIdentifierAndColour&&) = delete;
	};

	using HasIdentifierAndColour = _HasIdentifierAndColour<colour_t>;
	using HasIdentifierAndAlphaColour = _HasIdentifierAndColour<colour_argb_t>;

	template<typename T>
	concept HasGetIdentifierAndGetColour = HasGetIdentifier<T> && HasGetColour<T>;

	template<typename T>
	concept HasGetName = requires(T const& t) {
		{ t.get_name() } -> std::same_as<std::string_view>;
	};
}

template<OpenVic::HasGetIdentifier T>
requires (!OpenVic::HasGetName<T>)
struct fmt::formatter<T> : fmt::formatter<fmt::string_view> {
	fmt::format_context::iterator format(T const& has_id, fmt::format_context& ctx) const {
		return fmt::formatter<fmt::string_view>::format(has_id.get_identifier(), ctx);
	}
};

template<OpenVic::HasGetName T>
requires (!OpenVic::HasGetIdentifier<T>)
struct fmt::formatter<T> : fmt::formatter<fmt::string_view> {
	fmt::format_context::iterator format(T const& has_id, fmt::format_context& ctx) const {
		return fmt::formatter<fmt::string_view>::format(has_id.get_name(), ctx);
	}
};