#pragma once

#include <concepts>
#include <variant>

#include "openvic-simulation/modifier/ModifierValue.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct CountryInstance;
	struct ProvinceInstance;
	struct Modifier;

	struct ModifierSum {
		using modifier_source_t = std::variant<CountryInstance const*, ProvinceInstance const*>;

		static std::string_view source_to_string(modifier_source_t const& source);

		struct modifier_entry_t {
			Modifier const& modifier;
			fixed_point_t multiplier;
			modifier_source_t source;
			ModifierEffect::target_t excluded_targets;

			constexpr modifier_entry_t(
				Modifier const& new_modifier,
				fixed_point_t new_multiplier,
				modifier_source_t const& new_source,
				ModifierEffect::target_t new_excluded_targets
			) : modifier { new_modifier },
				multiplier { new_multiplier },
				source { new_source },
				excluded_targets { new_excluded_targets } {}

			constexpr bool operator==(modifier_entry_t const& other) const {
				return &modifier == &other.modifier
					&& multiplier == other.multiplier
					&& source == other.source
					&& excluded_targets == other.excluded_targets;
			}

			std::string to_string() const;

			fixed_point_t get_modifier_effect_value(ModifierEffect const& effect) const;
			fixed_point_t get_modifier_effect_value_nullcheck(ModifierEffect const* effect) const;
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
			ModifierEffect::target_t excluded_targets = ModifierEffect::target_t::NO_TARGETS
		);
		void add_modifier_nullcheck(
			Modifier const* modifier, modifier_source_t const& source, fixed_point_t multiplier = fixed_point_t::_1(),
			ModifierEffect::target_t excluded_targets = ModifierEffect::target_t::NO_TARGETS
		);
		void add_modifier_sum_exclude_targets(ModifierSum const& modifier_sum, ModifierEffect::target_t excluded_targets);
		void add_modifier_sum_exclude_source(ModifierSum const& modifier_sum, modifier_source_t const& excluded_source);

		static constexpr bool is_contributing(ModifierEffect const& effect, modifier_entry_t const& modifier_entry) {
			return ModifierEffect::excludes_targets(effect.get_targets(), modifier_entry.excluded_targets) &&
				modifier_entry.modifier.has_effect(effect);;
		}
		// TODO - help calculate value_sum[effect]? Early return if lookup in value_sum fails?
		constexpr void for_each_contributing_modifier(
			ModifierEffect const& effect, std::invocable<modifier_entry_t const&> auto callback
		) const {
			for (modifier_entry_t const& modifier_entry : modifiers) {
				if (is_contributing(effect, modifier_entry)) {
					callback(modifier_entry);
				}
			}
		}
	};
}
