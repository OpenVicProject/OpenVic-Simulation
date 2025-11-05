#pragma once

#include "openvic-simulation/core/Property.hpp"
#include "openvic-simulation/core/object/Colour.hpp"

namespace OpenVic {
	/*
	 * Base class for objects with associated colour information.
	 */
	template<IsColour ColourT>
	class _HasColour {
		const ColourT PROPERTY(colour);

	protected:
		_HasColour(ColourT new_colour, bool cannot_be_null) : colour { new_colour } {
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
}
