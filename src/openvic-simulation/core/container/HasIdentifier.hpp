#pragma once

#include <cassert>
#include <ostream>
#include <string_view>

#include <fmt/base.h>

#include "openvic-simulation/core/Property.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"

namespace OpenVic {

	/*
	 * Base class for objects with a non-empty string identifier. Uniquely named instances of a type derived from this class
	 * can be entered into an IdentifierRegistry instance.
	 */
	class HasIdentifier {
		/* Not const so it can be moved rather than needing to be copied. */
		memory::string PROPERTY(identifier);

	protected:
		HasIdentifier(std::string_view new_identifier) : identifier { new_identifier } {
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
}

template<OpenVic::has_get_identifier T>
requires(!OpenVic::has_get_name<T>)
struct fmt::formatter<T> : fmt::formatter<fmt::string_view> {
	fmt::format_context::iterator format(T const& has_id, fmt::format_context& ctx) const {
		return fmt::formatter<fmt::string_view>::format(has_id.get_identifier(), ctx);
	}
};

template<OpenVic::has_get_name T>
requires(!OpenVic::has_get_identifier<T>)
struct fmt::formatter<T> : fmt::formatter<fmt::string_view> {
	fmt::format_context::iterator format(T const& has_id, fmt::format_context& ctx) const {
		return fmt::formatter<fmt::string_view>::format(has_id.get_name(), ctx);
	}
};
