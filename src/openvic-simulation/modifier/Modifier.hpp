#pragma once

#include "openvic-simulation/modifier/ModifierEffect.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/modifier/ModifierValue.hpp"
#include "openvic-simulation/modifier/StaticModifierCache.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct Modifier : HasIdentifier, ModifierValue {
		friend struct ModifierManager;

		enum struct modifier_type_t : uint8_t {
			EVENT, STATIC, TRIGGERED, CRIME, TERRAIN, CLIMATE, CONTINENT, BUILDING, LEADER, NATIONAL_VALUE, NATIONAL_FOCUS,
			ISSUE, REFORM, TECHNOLOGY, INVENTION, TECH_SCHOOL
		};

	private:
		const modifier_type_t PROPERTY(type);

	protected:
		Modifier(std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type);

	public:
		Modifier(Modifier&&) = default;
	};

	struct IconModifier : Modifier {
		friend struct ModifierManager;

		using icon_t = uint8_t;

	private:
		/* A modifier can have no icon (zero). */
		const icon_t PROPERTY(icon);

	protected:
		IconModifier(std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon);

	public:
		IconModifier(IconModifier&&) = default;
	};

	struct TriggeredModifier : IconModifier {
		friend struct ModifierManager;

	private:
		ConditionScript trigger;

	protected:
		TriggeredModifier(
			std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon,
			ConditionScript&& new_trigger
		);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		TriggeredModifier(TriggeredModifier&&) = default;
	};

	struct ModifierInstance {

	private:
		Modifier const* PROPERTY(modifier); // We can assume this is never null
		Date PROPERTY(expiry_date);

	public:
		ModifierInstance(Modifier const& new_modifier, Date new_expiry_date);
	};

	template<typename Fn>
	concept ModifierEffectValidator = std::predicate<Fn, ModifierEffect const&>;

	struct ModifierManager {
		friend struct StaticModifierCache;
		friend struct BuildingTypeManager;
		friend struct GoodDefinitionManager;
		friend struct UnitTypeManager;
		friend struct RebelManager;
		friend struct PopManager;
		friend struct TechnologyManager;

		/* Some ModifierEffects are generated mid-load, such as max/min count modifiers for each building, so
		 * we can't lock it until loading is over. This means we can't rely on locking for pointer stability,
		 * so instead we store the effects in a deque which doesn't invalidate pointers on insert.
		 */
	private:
		CaseInsensitiveIdentifierRegistry<ModifierEffect, RegistryStorageInfoDeque> IDENTIFIER_REGISTRY(modifier_effect);
		case_insensitive_string_set_t complex_modifiers;

		IdentifierRegistry<IconModifier> IDENTIFIER_REGISTRY(event_modifier);
		IdentifierRegistry<Modifier> IDENTIFIER_REGISTRY(static_modifier);
		IdentifierRegistry<TriggeredModifier> IDENTIFIER_REGISTRY(triggered_modifier);

		ModifierEffectCache PROPERTY(modifier_effect_cache);
		StaticModifierCache PROPERTY(static_modifier_cache);

		/* effect_validator takes in ModifierEffect const& */
		NodeTools::key_value_callback_t _modifier_effect_callback(
			ModifierValue& modifier, NodeTools::key_value_callback_t default_callback,
			ModifierEffectValidator auto effect_validator
		) const;

	public:
		bool add_modifier_effect(
			ModifierEffect const*& effect_cache,
			std::string_view identifier,
			bool positive_good,
			ModifierEffect::format_t format,
			ModifierEffect::target_t targets,
			std::string_view localisation_key = {}
		);

		bool register_complex_modifier(std::string_view identifier);
		static std::string get_flat_identifier(
			std::string_view complex_modifier_identifier, std::string_view variant_identifier
		);

		bool setup_modifier_effects();

		bool add_event_modifier(std::string_view identifier, ModifierValue&& values, IconModifier::icon_t icon);
		bool load_event_modifiers(ast::NodeCPtr root);

		bool add_static_modifier(std::string_view identifier, ModifierValue&& values);
		bool load_static_modifiers(ast::NodeCPtr root);

		bool add_triggered_modifier(
			std::string_view identifier, ModifierValue&& values, IconModifier::icon_t icon, ConditionScript&& trigger
		);
		bool load_triggered_modifiers(ast::NodeCPtr root);

		bool parse_scripts(DefinitionManager const& definition_manager);

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
