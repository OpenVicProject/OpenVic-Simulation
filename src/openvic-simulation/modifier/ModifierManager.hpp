#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/modifier/StaticModifierCache.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {

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

		using effect_variant_map_t = ordered_map<Modifier::modifier_type_t, ModifierEffect const*>;

		/* Some ModifierEffects are generated mid-load, such as max/min count modifiers for each building, so
		 * we can't lock it until loading is over. This means we can't rely on locking for pointer stability,
		 * so instead we store the effects in a deque which doesn't invalidate pointers on insert.
		 */
	private:
		CaseInsensitiveIdentifierRegistry<ModifierEffect, RegistryStorageInfoDeque> IDENTIFIER_REGISTRY(modifier_effect);
		case_insensitive_string_set_t complex_modifiers;
		string_map_t<effect_variant_map_t> modifier_effect_variants;

		IdentifierRegistry<IconModifier> IDENTIFIER_REGISTRY(event_modifier);
		IdentifierRegistry<Modifier> IDENTIFIER_REGISTRY(static_modifier);
		IdentifierRegistry<TriggeredModifier> IDENTIFIER_REGISTRY(triggered_modifier);

		ModifierEffectCache PROPERTY(modifier_effect_cache);
		StaticModifierCache PROPERTY(static_modifier_cache);

		/* effect_validator takes in ModifierEffect const& */
		NodeTools::key_value_callback_t _modifier_effect_callback(
			ModifierValue& modifier, Modifier::modifier_type_t type, NodeTools::key_value_callback_t default_callback,
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

		bool register_modifier_effect_variants(
			std::string const& identifier, ModifierEffect const* effect, std::vector<Modifier::modifier_type_t> const& types
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
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type,
			NodeTools::key_value_callback_t default_callback, ModifierEffectValidator auto effect_validator
		) const;
		NodeTools::node_callback_t expect_validated_modifier_value(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type,
			ModifierEffectValidator auto effect_validator
		) const;

		NodeTools::node_callback_t expect_modifier_value_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type,
			NodeTools::key_value_callback_t default_callback
		) const;
		NodeTools::node_callback_t expect_modifier_value(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type
		) const;

		NodeTools::node_callback_t expect_whitelisted_modifier_value_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type,
			string_set_t const& whitelist, NodeTools::key_value_callback_t default_callback
		) const;
		NodeTools::node_callback_t expect_whitelisted_modifier_value(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type,
			string_set_t const& whitelist
		) const;

		// In the functions below, key_map refers to a map from identifier strings to NodeTools::dictionary_entry_t,
		// allowing any non-modifier effect keys found to be parsed in a custom way, similar to expect_dictionary_keys.
		NodeTools::node_callback_t expect_modifier_value_and_key_map_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type,
			NodeTools::key_value_callback_t default_callback, NodeTools::key_map_t&& key_map
		) const;
		NodeTools::node_callback_t expect_modifier_value_and_key_map(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type,
			NodeTools::key_map_t&& key_map
		) const;

		template<typename... Args>
		NodeTools::node_callback_t expect_modifier_value_and_key_map_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type,
			NodeTools::key_value_callback_t default_callback, NodeTools::key_map_t&& key_map, Args... args
		) const {
			NodeTools::add_key_map_entries(key_map, args...);
			return expect_modifier_value_and_key_map_and_default(
				modifier_callback, type, default_callback, std::move(key_map)
			);
		}

		template<typename... Args>
		NodeTools::node_callback_t expect_modifier_value_and_keys_and_default(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type,
			NodeTools::key_value_callback_t default_callback, Args... args
		) const {
			return expect_modifier_value_and_key_map_and_default(modifier_callback, type, default_callback, {}, args...);
		}
		template<typename... Args>
		NodeTools::node_callback_t expect_modifier_value_and_keys(
			NodeTools::callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type, Args... args
		) const {
			return expect_modifier_value_and_key_map_and_default(
				modifier_callback, type, NodeTools::key_value_invalid_callback, {}, args...
			);
		}
	};
}
