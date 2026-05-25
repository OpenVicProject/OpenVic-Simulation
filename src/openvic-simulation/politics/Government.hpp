#pragma once

#include <functional>

#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct Ideology;
	struct IdeologyManager;

	struct GovernmentType : HasIndex<GovernmentType, government_type_index_t>, HasIdentifier {
	private:
		memory::vector<std::reference_wrapper<const Ideology>> SPAN_PROPERTY(ideologies);
		memory::string PROPERTY_CUSTOM_NAME(flag_type_identifier, get_flag_type);

	public:
		const bool holds_elections;
		const bool can_appoint_ruling_party;
		const Timespan term_duration;

		GovernmentType(
			index_t new_index,
			std::string_view new_identifier,
			memory::vector<std::reference_wrapper<const Ideology>>&& new_ideologies,
			bool new_holds_elections,
			bool new_can_appoint_ruling_party,
			Timespan new_term_duration,
			std::string_view new_flag_type_identifier
		);
		GovernmentType(GovernmentType&&) = default;

		bool is_ideology_compatible(Ideology const& ideology) const;
	};
}
