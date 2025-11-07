#pragma once

#include <string_view>

#include "openvic-simulation/core/container/IdentifierRegistry.hpp"
#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/memory/StringSet.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/modifier/StaticModifierCache.hpp"

namespace OpenVic {
	struct ModifierManager {
		friend struct StaticModifierCache;
		friend struct BuildingTypeManager;
		friend struct GoodDefinitionManager;
		friend struct UnitTypeManager;
		friend struct RebelManager;
		friend struct PopManager;
		friend struct TechnologyManager;
		friend struct TerrainTypeManager;

		using modifier_effect_registry_t = CaseInsensitiveIdentifierRegistry<ModifierEffect, RegistryStorageInfoDeque>;

		/* Some ModifierEffects are generated mid-load, such as max/min count modifiers for each building, so
		 * we can't lock it until loading is over. This means we can't rely on locking for pointer stability,
		 * so instead we store the effects in a deque which doesn't invalidate pointers on insert.
		 */
	private:
		modifier_effect_registry_t IDENTIFIER_REGISTRY(leader_modifier_effect);
		modifier_effect_registry_t IDENTIFIER_REGISTRY(unit_terrain_modifier_effect);
		modifier_effect_registry_t IDENTIFIER_REGISTRY(technology_modifier_effect);
		modifier_effect_registry_t IDENTIFIER_REGISTRY(base_country_modifier_effect);
		modifier_effect_registry_t IDENTIFIER_REGISTRY(base_province_modifier_effect);
		modifier_effect_registry_t IDENTIFIER_REGISTRY(terrain_modifier_effect);
		memory::case_insensitive_string_set_t complex_modifiers;

		IdentifierRegistry<IconModifier> IDENTIFIER_REGISTRY(event_modifier);
		IdentifierRegistry<TriggeredModifier> IDENTIFIER_REGISTRY(triggered_modifier);

		ModifierEffectCache PROPERTY(modifier_effect_cache);
		StaticModifierCache PROPERTY(static_modifier_cache);

		bool _register_modifier_effect(
			modifier_effect_registry_t& registry,
			ModifierEffect::target_t targets,
			ModifierEffect const*& effect_cache,
			const std::string_view identifier,
			const ModifierEffect::format_t format,
			const std::string_view localisation_key,
			const bool has_no_effect
		);

#define REGISTER_MODIFIER_EFFECT_DEFINITION(MAPPING_TYPE) \
		bool register_##MAPPING_TYPE##_modifier_effect( \
			ModifierEffect const*& effect_cache, \
			std::string_view identifier, \
			ModifierEffect::format_t format, \
			std::string_view localisation_key = {}, \
			bool has_no_effect = false \
		);

		REGISTER_MODIFIER_EFFECT_DEFINITION(leader)
		REGISTER_MODIFIER_EFFECT_DEFINITION(unit_terrain)
		REGISTER_MODIFIER_EFFECT_DEFINITION(technology)
		REGISTER_MODIFIER_EFFECT_DEFINITION(base_country)
		REGISTER_MODIFIER_EFFECT_DEFINITION(base_province)
		REGISTER_MODIFIER_EFFECT_DEFINITION(terrain)
#undef REGISTER_MODIFIER_EFFECT_DEFINITION

		bool _add_flattened_modifier_cb(
			ModifierValue& modifier_value,
			const std::string_view prefix,
			const std::string_view key,
			const ast::NodeCPtr value
		) const;

		bool _add_modifier_cb(
			ModifierValue& modifier_value,
			ModifierEffect const* const effect,
			const ast::NodeCPtr value
		) const;

		NodeTools::key_value_callback_t _expect_modifier_effect(
			modifier_effect_registry_t const& registry,
			ModifierValue& modifier_value
		) const;

		NodeTools::key_value_callback_t _expect_modifier_effect_with_fallback(
			modifier_effect_registry_t const& registry,
			ModifierValue& modifier_value,
			NodeTools::key_value_callback_t fallback
		) const;

	public:
		bool register_complex_modifier(const std::string_view identifier);
		static memory::string get_flat_identifier(const std::string_view complex_modifier_identifier, const std::string_view variant_identifier);

		bool setup_modifier_effects();

		bool add_event_modifier(
			const std::string_view identifier,
			ModifierValue&& values,
			const IconModifier::icon_t icon,
			const Modifier::modifier_type_t type = Modifier::modifier_type_t::EVENT
		);
		bool load_event_modifiers(const ast::NodeCPtr root);

		bool load_static_modifiers(const ast::NodeCPtr root);

		bool add_triggered_modifier(
			const std::string_view identifier,
			ModifierValue&& values,
			const IconModifier::icon_t icon,
			ConditionScript&& trigger
		);
		bool load_triggered_modifiers(const ast::NodeCPtr root);

		bool parse_scripts(DefinitionManager const& definition_manager);
		void lock_all_modifier_except_base_country_effects();

		NodeTools::key_value_callback_t expect_leader_modifier(ModifierValue& modifier_value) const;
		NodeTools::key_value_callback_t expect_technology_modifier(ModifierValue& modifier_value) const;
		NodeTools::key_value_callback_t expect_unit_terrain_modifier(
			ModifierValue& modifier_value,
			const std::string_view terrain_type_identifier
		) const;
		NodeTools::key_value_callback_t expect_base_country_modifier(ModifierValue& modifier_value) const;
		NodeTools::key_value_callback_t expect_base_province_modifier(ModifierValue& modifier_value) const;
		NodeTools::key_value_callback_t expect_terrain_modifier(ModifierValue& modifier_value) const;
	};
}
