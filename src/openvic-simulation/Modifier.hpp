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

		ModifierEffect(std::string_view new_identifier, bool new_positive_good);

	public:
		ModifierEffect(ModifierEffect&&) = default;

		bool get_positive_good() const;
	};

	struct ModifierValue {
		friend struct ModifierManager;

		using effect_map_t = std::map<ModifierEffect const*, fixed_point_t>;
	private:
		effect_map_t values;

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
		/* A modifier can have no icon (zero). */
		const icon_t icon;

		Modifier(std::string_view new_identifier, ModifierValue&& new_values, icon_t new_icon);

	public:
		Modifier(Modifier&&) = default;

		icon_t get_icon() const;
	};

	struct ModifierInstance {

	private:
		Modifier const& modifier;
		Date expiry_date;

		ModifierInstance(Modifier const& modifier, Date expiry_date);

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

		bool add_modifier_effect(std::string_view identifier, bool province_good);
		IDENTIFIER_REGISTRY_ACCESSORS(modifier_effect)

		bool add_modifier(std::string_view identifier, ModifierValue&& values, Modifier::icon_t icon);
		IDENTIFIER_REGISTRY_ACCESSORS(modifier)

		bool setup_modifier_effects();

		NodeTools::node_callback_t expect_modifier_value_and_default(NodeTools::callback_t<ModifierValue&&> modifier_callback, NodeTools::key_value_callback_t default_callback) const;
		NodeTools::node_callback_t expect_modifier_value(NodeTools::callback_t<ModifierValue&&> modifier_callback) const;

		NodeTools::node_callback_t expect_modifier_value_and_key_map_and_default(NodeTools::callback_t<ModifierValue&&> modifier_callback, NodeTools::key_value_callback_t default_callback, NodeTools::key_map_t&& key_map) const;
		NodeTools::node_callback_t expect_modifier_value_and_key_map(NodeTools::callback_t<ModifierValue&&> modifier_callback, NodeTools::key_map_t&& key_map) const;

		template<typename... Args>
		NodeTools::node_callback_t expect_modifier_value_and_key_map_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, NodeTools::key_value_callback_t default_callback, NodeTools::key_map_t&& key_map,
			std::string_view key, NodeTools::dictionary_entry_t::expected_count_t expected_count, NodeTools::node_callback_t callback,
			Args... args) const {
			NodeTools::add_key_map_entry(key_map, key, expected_count, callback);
			return expect_modifier_value_and_key_map_and_default(modifier_callback, default_callback, std::move(key_map), args...);
		}

		template<typename... Args>
		NodeTools::node_callback_t expect_modifier_value_and_keys_and_default(NodeTools::callback_t<ModifierValue&&> modifier_callback, NodeTools::key_value_callback_t default_callback, Args... args) const {
			return expect_modifier_value_and_key_map_and_default(modifier_callback, default_callback, {}, args...);
		}
		template<typename... Args>
		NodeTools::node_callback_t expect_modifier_value_and_keys(NodeTools::callback_t<ModifierValue&&> modifier_callback, Args... args) const {
			return expect_modifier_value_and_key_map_and_default(modifier_callback, NodeTools::key_value_invalid_callback, {}, args...);
		}
	};
}
