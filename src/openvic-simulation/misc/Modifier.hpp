#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct ModifierManager;

	struct ModifierEffect : HasIdentifier {
		enum class format_t {
			PROPORTION_DECIMAL,	/* An unscaled fraction/ratio, with 1 being "full"/"whole" */
			PERCENTAGE_DECIMAL,	/* A fraction/ratio scaled so that 100 is "full"/"whole" */
			RAW_DECIMAL,		/* A continuous quantity, e.g. attack strength */
			INT					/* A discrete quantity, e.g. building count limit */
		};

		friend std::unique_ptr<ModifierEffect> std::make_unique<ModifierEffect>(std::string_view&&, bool&&, format_t&&);

	private:
		/* If true, positive values will be green and negative values will be red.
		 * If false, the colours will be switced.
		 */
		const bool PROPERTY_CUSTOM_PREFIX(positive_good, is);
		const format_t PROPERTY(format);

		// TODO - format/precision, e.g. 80% vs 0.8 vs 0.800, 2 vs 2.0 vs 200%

		ModifierEffect(std::string_view new_identifier, bool new_positive_good, format_t new_format);

	public:
		ModifierEffect(ModifierEffect&&) = default;
	};

	struct ModifierValue {
		friend struct ModifierManager;

		using effect_map_t = fixed_point_map_t<ModifierEffect const*>;

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
		const icon_t PROPERTY(icon);

	protected:
		Modifier(std::string_view new_identifier, ModifierValue&& new_values, icon_t new_icon);

	public:
		Modifier(Modifier&&) = default;
	};

	struct TriggeredModifier : Modifier {
		friend struct ModifierManager;

	private:
		// TODO - trigger condition

	protected:
		TriggeredModifier(std::string_view new_identifier, ModifierValue&& new_values, icon_t new_icon);

	public:
		TriggeredModifier(TriggeredModifier&&) = default;
	};

	struct ModifierInstance {

	private:
		Modifier const& PROPERTY(modifier);
		Date PROPERTY(expiry_date);

		ModifierInstance(Modifier const& modifier, Date expiry_date);
	};

	template<typename Fn>
	concept ModifierEffectValidator = std::predicate<Fn, ModifierEffect const&>;

	struct ModifierManager {
		/* Some ModifierEffects are generated mid-load, such as max/min count modifiers for each building, so
		 * we can't lock it until loading is over. This means we can't rely on locking for pointer stability,
		 * so instead we use an IdentifierInstanceRegistry (using std::unique_ptr's under the hood).
		 */
	private:
		IdentifierInstanceRegistry<ModifierEffect> modifier_effects;
		string_set_t complex_modifiers;

		IdentifierRegistry<Modifier> event_modifiers;
		IdentifierRegistry<Modifier> static_modifiers;
		IdentifierRegistry<TriggeredModifier> triggered_modifiers;

		/* effect_validator takes in ModifierEffect const& */
		NodeTools::key_value_callback_t _modifier_effect_callback(
			ModifierValue& modifier, NodeTools::key_value_callback_t default_callback,
			ModifierEffectValidator auto effect_validator
		) const;

	public:
		ModifierManager();

		bool add_modifier_effect(
			std::string_view identifier, bool positive_good,
			ModifierEffect::format_t format = ModifierEffect::format_t::PROPORTION_DECIMAL
		);
		IDENTIFIER_REGISTRY_ACCESSORS(modifier_effect)

		void register_complex_modifier(std::string_view identifier);

		bool setup_modifier_effects();

		bool add_event_modifier(std::string_view identifier, ModifierValue&& values, Modifier::icon_t icon);
		IDENTIFIER_REGISTRY_ACCESSORS(event_modifier)
		bool load_event_modifiers(ast::NodeCPtr root);

		bool add_static_modifier(std::string_view identifier, ModifierValue&& values);
		IDENTIFIER_REGISTRY_ACCESSORS(static_modifier)
		bool load_static_modifiers(ast::NodeCPtr root);

		bool add_triggered_modifier(std::string_view identifier, ModifierValue&& values, Modifier::icon_t icon);
		IDENTIFIER_REGISTRY_ACCESSORS(triggered_modifier)
		bool load_triggered_modifiers(ast::NodeCPtr root);

		NodeTools::node_callback_t expect_validated_modifier_value_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, NodeTools::key_value_callback_t default_callback,
			ModifierEffectValidator auto effect_validator
		) const;
		NodeTools::node_callback_t expect_validated_modifier_value(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, ModifierEffectValidator auto effect_validator
		) const;

		NodeTools::node_callback_t expect_modifier_value_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, NodeTools::key_value_callback_t default_callback
		) const;
		NodeTools::node_callback_t expect_modifier_value(NodeTools::callback_t<ModifierValue&&> modifier_callback) const;

		NodeTools::node_callback_t expect_whitelisted_modifier_value_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, string_set_t const& whitelist,
			NodeTools::key_value_callback_t default_callback
		) const;
		NodeTools::node_callback_t expect_whitelisted_modifier_value(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, string_set_t const& whitelist
		) const;

		NodeTools::node_callback_t expect_modifier_value_and_key_map_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, NodeTools::key_value_callback_t default_callback,
			NodeTools::key_map_t&& key_map
		) const;
		NodeTools::node_callback_t expect_modifier_value_and_key_map(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, NodeTools::key_map_t&& key_map
		) const;

		template<typename... Args>
		NodeTools::node_callback_t expect_modifier_value_and_key_map_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, NodeTools::key_value_callback_t default_callback,
			NodeTools::key_map_t&& key_map, Args... args
		) const {
			NodeTools::add_key_map_entries(key_map, args...);
			return expect_modifier_value_and_key_map_and_default(modifier_callback, default_callback, std::move(key_map));
		}

		template<typename... Args>
		NodeTools::node_callback_t expect_modifier_value_and_keys_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, NodeTools::key_value_callback_t default_callback,
			Args... args
		) const {
			return expect_modifier_value_and_key_map_and_default(modifier_callback, default_callback, {}, args...);
		}
		template<typename... Args>
		NodeTools::node_callback_t expect_modifier_value_and_keys(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Args... args
		) const {
			return expect_modifier_value_and_key_map_and_default(
				modifier_callback, NodeTools::key_value_invalid_callback, {}, args...
			);
		}
	};
}
