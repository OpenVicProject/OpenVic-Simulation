#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"

namespace OpenVic {
	struct CountryInstance;
	struct ProvinceInstance;

	struct ModifierSum {
		using modifier_source_t = std::variant<CountryInstance const*, ProvinceInstance const*>;

		struct modifier_entry_t {
			Modifier const* modifier;
			fixed_point_t multiplier;
			modifier_source_t source;

			constexpr modifier_entry_t(
				Modifier const* new_modifier, fixed_point_t new_multiplier, modifier_source_t const& new_source
			) : modifier { new_modifier }, multiplier { new_multiplier }, source { new_source } {}
		};

	private:
		std::vector<modifier_entry_t> PROPERTY(modifiers);
		ModifierValue PROPERTY(value_sum);

	public:
		ModifierSum() = default;
		ModifierSum(ModifierSum&&) = default;

		void clear();
		bool empty();

		fixed_point_t get_effect(ModifierEffect const& effect, bool* effect_found = nullptr) const;
		bool has_effect(ModifierEffect const& effect) const;

		void add_modifier(Modifier const& modifier, modifier_source_t source, fixed_point_t multiplier = fixed_point_t::_1());
		void add_modifier_sum(ModifierSum const& modifier_sum);
		void add_modifier_sum_exclude_source(ModifierSum const& modifier_sum, modifier_source_t const& excluded_source);

		std::vector<modifier_entry_t> get_contributing_modifiers(ModifierEffect const& effect) const;
	};
}
