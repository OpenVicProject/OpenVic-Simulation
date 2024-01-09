#pragma once

#include <algorithm>
#include <cassert>
#include <ostream>

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
		const std::string PROPERTY(identifier);

	protected:
		HasIdentifier(std::string_view new_identifier): identifier { new_identifier } {
			assert(!identifier.empty());
		}

	public:
		HasIdentifier(HasIdentifier const&) = delete;
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

	public:
		_HasColour(_HasColour const&) = delete;
		_HasColour(_HasColour&&) = default;
		_HasColour& operator=(_HasColour const&) = delete;
		_HasColour& operator=(_HasColour&&) = delete;
	};

	/*
	 * Base class for objects with a unique string identifier and associated colour information.
	 */
	template<IsColour ColourT>
	class _HasIdentifierAndColour : public HasIdentifier, public _HasColour<ColourT> {
	protected:
		_HasIdentifierAndColour(std::string_view new_identifier, ColourT new_colour, bool cannot_be_null)
			: HasIdentifier { new_identifier }, _HasColour<ColourT> { new_colour, cannot_be_null } {}

	public:
		_HasIdentifierAndColour(_HasIdentifierAndColour const&) = delete;
		_HasIdentifierAndColour(_HasIdentifierAndColour&&) = default;
		_HasIdentifierAndColour& operator=(_HasIdentifierAndColour const&) = delete;
		_HasIdentifierAndColour& operator=(_HasIdentifierAndColour&&) = delete;
	};

	using HasIdentifierAndColour = _HasIdentifierAndColour<colour_t>;
	using HasIdentifierAndAlphaColour = _HasIdentifierAndColour<colour_argb_t>;
}
