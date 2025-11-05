#pragma once

#include "openvic-simulation/core/object/Colour.hpp"
#include "openvic-simulation/core/container/HasIdentifier.hpp"
#include "openvic-simulation/core/container/HasColour.hpp"

namespace OpenVic {
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
}