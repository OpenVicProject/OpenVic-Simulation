#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"

namespace OpenVic {
	struct ModifierSum {
	private:
		fixed_point_map_t<Modifier const*> PROPERTY(modifiers);
		ModifierValue PROPERTY(value_sum);

	public:
		ModifierSum() = default;
		ModifierSum(ModifierSum&&) = default;

		void clear();
		bool empty();

		fixed_point_t get_effect(ModifierEffect const& effect, bool* effect_found = nullptr) const;
		bool has_effect(ModifierEffect const& effect) const;

		void add_modifier(Modifier const& modifier, fixed_point_t multiplier = fixed_point_t::_1());
		void add_modifier_sum(ModifierSum const& modifier_sum);

		ModifierSum& operator+=(Modifier const& modifier);
		ModifierSum& operator+=(ModifierSum const& modifier_sum);

		std::vector<std::pair<Modifier const*, fixed_point_t>> get_contributing_modifiers(ModifierEffect const& effect) const;
	};
}
