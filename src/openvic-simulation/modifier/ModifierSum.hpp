#pragma once

#include <cstddef>
#include <variant>

#include "openvic-simulation/core/BulkInsertWrapper.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/core/portable/ForwardableSpan.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/modifier/ModifierValue.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct CountryInstance;
	struct ProvinceInstance;
	struct Modifier;

	struct modifier_entry_t {
		using modifier_source_t = std::variant<CountryInstance const*, ProvinceInstance const*>;

		static std::string_view source_to_string(modifier_source_t const& source);
		static constexpr bool source_is_null(modifier_source_t const& source) {
			return std::visit([](auto const* source) -> bool { return source == nullptr; }, source);
		}
		static constexpr modifier_source_t const& source_or_null_fallback(
			modifier_source_t const& source, modifier_source_t const& fallback
		) {
			return std::visit(
				[&source, &fallback](auto const* source_ptr) -> modifier_source_t const& {
					return source_ptr == nullptr ? fallback : source;
				},
				source
			);
		}

		Modifier const* modifier;
		fixed_point_t multiplier;
		modifier_source_t source;
		ModifierEffect::target_t excluded_targets;

		//invalid but required fore resizing to work
		constexpr modifier_entry_t()
			: modifier {nullptr},
			multiplier {fixed_point_t::_0},
		 	source {static_cast<std::variant_alternative_t<0, modifier_source_t>>(nullptr)},
			excluded_targets {} {}

		//
		constexpr modifier_entry_t(
			Modifier const& new_modifier,
			fixed_point_t new_multiplier,
			modifier_source_t const& new_source,
			ModifierEffect::target_t new_excluded_targets
		) : modifier { &new_modifier },
			multiplier { new_multiplier },
			source { new_source },
			excluded_targets { new_excluded_targets } {}

		constexpr bool is_valid() const {
			return multiplier != fixed_point_t::_0
				&& modifier != nullptr
				&& (
					get_source_country() != nullptr
					|| get_source_province() != nullptr
				);
		}

		constexpr bool operator==(modifier_entry_t const& other) const {
			return modifier == other.modifier
				&& multiplier == other.multiplier
				&& source == other.source
				&& excluded_targets == other.excluded_targets;
		}

		constexpr CountryInstance const* get_source_country() const {
			CountryInstance const* const* country = std::get_if<CountryInstance const*>(&source);
			return country != nullptr ? *country : nullptr;
		}
		constexpr ProvinceInstance const* get_source_province() const {
			ProvinceInstance const* const* province = std::get_if<ProvinceInstance const*>(&source);
			return province != nullptr ? *province : nullptr;
		}

		memory::string to_string() const;

		constexpr fixed_point_t get_modifier_effect_value(
			ModifierEffect const& effect, bool* effect_found = nullptr
		) const {
			if (!is_valid()) {
				return fixed_point_t::_0;
			}

			if (ModifierEffect::excludes_targets(effect.targets, excluded_targets)) {
				return modifier->get_effect(effect, effect_found) * multiplier;
			}

			if (effect_found != nullptr) {
				*effect_found = false;
			}
			return fixed_point_t::_0;
		}
	};

	// A callback like this is called for each entry in a modifier sum that contributes to a specific effect's resultant value.
	// The arguments are a modifier entry and the fixed point value of the effect given by that entry. The value could be
	// calculated from the entry and the effect we're looking at using "modifier_entry.get_modifier_effect_value(effect)",
	// but this has a non-negligible cost and we already calculate it as part of the "is_contributing" checks, so we pass
	// the pre-calculated value along as a callback argument.
	template<typename T>
	concept ContributingModifierCallback = NodeTools::Functor<T, void, modifier_entry_t const&, fixed_point_t>;

	struct ModifierSum {
	private:
		// Default source used for any modifier added to the sum without an explicit non-null source,
		// representing the object the sum is attached to.
		modifier_entry_t::modifier_source_t PROPERTY_RW(this_source);
		// Targets to be excluded from all modifiers added to the sum, combined with any explicit exclusions.
		ModifierEffect::target_t PROPERTY_RW(this_excluded_targets, ModifierEffect::target_t::NO_TARGETS);

		bulk_insert_wrapper<
			memory::vector<modifier_entry_t>
		> SPAN_PROPERTY(modifiers);
		ModifierValue PROPERTY(value_sum);

	public:
		ModifierSum() {};
		ModifierSum(ModifierSum const&) = default;
		ModifierSum(ModifierSum&&) = default;
		ModifierSum& operator=(ModifierSum const&) = default;
		ModifierSum& operator=(ModifierSum&&) = default;

		constexpr std::size_t size() const {
			return modifiers.size();
		}
		constexpr bool empty() const {
			return modifiers.empty();
		}
		void clear();

		fixed_point_t get_modifier_effect_value(ModifierEffect const& effect, bool* effect_found = nullptr) const;
		bool has_modifier_effect(ModifierEffect const& effect) const;

		void add_modifier(
			Modifier const& modifier,
			fixed_point_t multiplier = 1,
			modifier_entry_t::modifier_source_t const& source = {},
			ModifierEffect::target_t excluded_targets = ModifierEffect::target_t::NO_TARGETS
		);

		constexpr void make_room_for(ModifierSum const& modifier_sum) {
			modifiers.make_room_for(modifier_sum.size());
		}

		// Inserts modifiers directly via std::ranges::copy. Requires resizing beforehand!
		// with the modifier entries' attributes as arguments. This means non-null sources are preserved (null ones are
		// replaced with this_source, but in practice the other sum should've set them itself already) and exclusion targets
		// are combined with this_excluded_targets.
		constexpr void add_modifier_sum(ModifierSum const& modifier_sum) {
			// We could test that excluded_targets != ALL_TARGETS, but in practice it's always
			// called with an explcit/hardcoded value and so won't ever exclude everything.
			modifiers.append_range(modifier_sum.modifiers);

			for (modifier_entry_t const& m : modifier_sum.get_modifiers()) {
				value_sum.multiply_add_exclude_targets(
					*m.modifier,
					m.multiplier,
					m.excluded_targets
				);
			}
		}

		// TODO - help calculate value_sum[effect]? Early return if lookup in value_sum fails?
		constexpr void for_each_contributing_modifier(
			ModifierEffect const& effect, ContributingModifierCallback auto callback
		) const {
			for (modifier_entry_t const& modifier_entry : get_modifiers()) {
				const fixed_point_t contribution = modifier_entry.get_modifier_effect_value(effect);

				if (contribution != 0) {
					callback(modifier_entry, contribution);
				}
			}
		}
	};
}
