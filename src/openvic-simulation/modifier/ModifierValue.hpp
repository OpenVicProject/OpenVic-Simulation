#pragma once

#include "openvic-simulation/modifier/ModifierEffect.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"

namespace OpenVic {
	struct ModifierValue {
		friend struct ModifierManager;

		using effect_map_t = fixed_point_map_t<ModifierEffect const*>;

	private:
		effect_map_t PROPERTY(values);

	public:
		ModifierValue();
		ModifierValue(effect_map_t&& new_values);
		ModifierValue(ModifierValue const&);
		ModifierValue(ModifierValue&&);

		ModifierValue& operator=(ModifierValue const&);
		ModifierValue& operator=(ModifierValue&&);

		/* Removes effect entries with a value of zero. */
		void trim();
		size_t get_effect_count() const;
		void clear();
		bool empty() const;

		fixed_point_t get_effect(ModifierEffect const& effect, bool* effect_found = nullptr) const;
		fixed_point_t get_effect_nullcheck(ModifierEffect const* effect, bool* effect_found = nullptr) const;
		bool has_effect(ModifierEffect const& effect) const;
		void set_effect(ModifierEffect const& effect, fixed_point_t value);

		ModifierValue& operator+=(ModifierValue const& right);
		ModifierValue operator+(ModifierValue const& right) const;
		ModifierValue operator-() const;
		ModifierValue& operator-=(ModifierValue const& right);
		ModifierValue operator-(ModifierValue const& right) const;
		ModifierValue& operator*=(fixed_point_t const& right);
		ModifierValue operator*(fixed_point_t const& right) const;

		void apply_exclude_targets(ModifierEffect::target_t excluded_targets);
		void multiply_add_exclude_targets(
			ModifierValue const& other, fixed_point_t multiplier, ModifierEffect::target_t excluded_targets
		);

		friend std::ostream& operator<<(std::ostream& stream, ModifierValue const& value);
	};
}
