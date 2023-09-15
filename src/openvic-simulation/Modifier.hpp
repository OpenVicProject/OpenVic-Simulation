#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct ModifierManager;

	struct ModifierEffect : HasIdentifier {
		friend struct ModifierManager;

	private:
		/* If true, positive values will be green and negative values will be red.
		 * If false, the colours will be switced.
		 */
		const bool positive_good;

		// TODO - format/precision, e.g. 80% vs 0.8 vs 0.800, 2 vs 2.0 vs 200%

		ModifierEffect(const std::string_view new_identifier, bool new_positive_good);
	
	public:
		ModifierEffect(ModifierEffect&&) = default;

		bool get_positive_good() const;
	};

	struct ModifierValue {
		using effect_map_t = std::map<ModifierEffect const*, fixed_point_t>;
	private:
		effect_map_t values;

	public:
		ModifierValue();
		ModifierValue(ModifierValue const&);
		ModifierValue(ModifierValue&&);

		/* Removes effect entries with a value of zero. */
		void trim();
		size_t get_effect_count() const;

		fixed_point_t get_effect(ModifierEffect const* effect, bool* successful = nullptr);
		bool has_effect(ModifierEffect const* effect) const;

		ModifierValue& operator+=(ModifierValue const& right);
		ModifierValue operator+(ModifierValue const& right) const;
		ModifierValue operator-() const;
		ModifierValue& operator-=(ModifierValue const& right);
		ModifierValue operator-(ModifierValue const& right) const;

		friend std::ostream& operator<<(std::ostream& stream, ModifierValue const& value);
	};

	struct Modifier : HasIdentifier, ModifierValue {
		friend struct ModifierManager;

		using icon_t = uint8_t;

	private:
		const icon_t icon;

		Modifier(const std::string_view new_identifier, ModifierValue&& new_values, icon_t new_icon);

	public:
		Modifier(Modifier&&) = default;

		icon_t get_icon() const;
	};

	struct ModifierInstance {

	private:
		Modifier const& modifier;
		Date expiry_date;

	public:
		Modifier const& get_modifier() const;
		Date const& get_expiry_date() const;
	};

	struct ModifierManager {

	private:
		IdentifierRegistry<ModifierEffect> modifier_effects;
		IdentifierRegistry<Modifier> modifiers;

	public:
		ModifierManager();

		bool add_modifier_effect(const std::string_view identifier, bool province_good);
		IDENTIFIER_REGISTRY_ACCESSORS(ModifierEffect, modifier_effect)

		bool add_modifier(const std::string_view identifier, ModifierValue&& values, Modifier::icon_t icon);
		IDENTIFIER_REGISTRY_ACCESSORS(Modifier, modifier)
	};
}
