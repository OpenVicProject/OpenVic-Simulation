#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct CrimeManager;

	struct Crime final : HasIndex<Crime, crime_index_t>, TriggeredModifier {
		friend struct CrimeManager;
	public:
		bool is_default_active;

		Crime(
			index_t new_index,
			std::string_view new_identifier,
			ModifierValue&& new_values,
			icon_t new_icon,
			ConditionScript&& new_trigger,
			bool new_default_active
		);
		Crime(Crime&&) = default;
	};
}
