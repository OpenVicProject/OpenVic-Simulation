#pragma once

#include <variant>

#include "openvic-simulation/modifier/ModifierValue.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct CountryInstance;
	struct ProvinceInstance;
	struct Modifier;

	struct ModifierSum {
		using modifier_source_t = std::variant<CountryInstance const*, ProvinceInstance const*>;

		struct modifier_entry_t {
			Modifier const* modifier;
			fixed_point_t multiplier;
			modifier_source_t source;
			ModifierEffect::target_t targets;

			constexpr modifier_entry_t(
				Modifier const* new_modifier, fixed_point_t new_multiplier, modifier_source_t const& new_source,
				ModifierEffect::target_t new_targets
			) : modifier { new_modifier }, multiplier { new_multiplier }, source { new_source }, targets { new_targets } {}
		};

	private:
		std::vector<modifier_entry_t> PROPERTY(modifiers);
		ModifierValue PROPERTY(value_sum);

	public:
		ModifierSum() = default;
		ModifierSum(ModifierSum const&) = default;
		ModifierSum(ModifierSum&&) = default;
		ModifierSum& operator=(ModifierSum const&) = default;
		ModifierSum& operator=(ModifierSum&&) = default;

		void clear();
		bool empty();

		fixed_point_t get_effect(ModifierEffect const& effect, bool* effect_found = nullptr) const;
		fixed_point_t get_effect_nullcheck(ModifierEffect const* effect, bool* effect_found = nullptr) const;
		bool has_effect(ModifierEffect const& effect) const;

		void add_modifier(
			Modifier const& modifier, modifier_source_t const& source, fixed_point_t multiplier = fixed_point_t::_1(),
			ModifierEffect::target_t targets = ModifierEffect::target_t::ALL_TARGETS
		);
		void add_modifier_nullcheck(
			Modifier const* modifier, modifier_source_t const& source, fixed_point_t multiplier = fixed_point_t::_1(),
			ModifierEffect::target_t targets = ModifierEffect::target_t::ALL_TARGETS
		);
		void add_modifier_sum(ModifierSum const& modifier_sum);
		void add_modifier_sum_filter_targets(ModifierSum const& modifier_sum, ModifierEffect::target_t targets);
		void add_modifier_sum_exclude_source(ModifierSum const& modifier_sum, modifier_source_t const& excluded_source);

		void push_contributing_modifiers(ModifierEffect const& effect, std::vector<modifier_entry_t>& contributions) const;
		std::vector<modifier_entry_t> get_contributing_modifiers(ModifierEffect const& effect) const;
	};
}
