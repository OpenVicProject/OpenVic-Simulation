#pragma once

#include "openvic-simulation/pop/Culture.hpp"
#include "openvic-simulation/pop/Religion.hpp"
#include "openvic-simulation/pop/PopType.hpp"

namespace OpenVic {
	struct GoodDefinitionManager;
	struct IdeologyManager;
	struct IssueManager;
	struct ModifierManager;
	struct PopBase;
	struct RebelManager;
	struct UnitTypeManager;

	struct PopManager {
	private:
		/* Using strata/stratas instead of stratum/strata to avoid confusion. */
		IdentifierRegistry<Strata> IDENTIFIER_REGISTRY(strata);
		IdentifierRegistry<PopType> IDENTIFIER_REGISTRY(pop_type);
		/* - rebel_units require UnitTypes which require on PopTypes (UnitType->Map->Building->ProductionType->PopType).
		 * - equivalent and promote_to can't be parsed until after all PopTypes are registered.
		 * - issues require Issues to be loaded, which themselves depend on pop strata.
		 * To get around these circular dependencies, the nodes for these variables are stored here and parsed after the
		 * necessary defines are loaded. The nodes will remain valid as PopType files' Parser objects are already cached to
		 * preserve their condition script nodes until all other defines are loaded and the scripts can be parsed.
		 * Entries contain: (rebel, equivalent, promote_to, issues) */
		memory::vector<std::tuple<ast::NodeCPtr, ast::NodeCPtr, ast::NodeCPtr, ast::NodeCPtr>> delayed_parse_nodes;

		ConditionalWeightFactorAdd PROPERTY(promotion_chance);
		ConditionalWeightFactorAdd PROPERTY(demotion_chance);
		ConditionalWeightFactorAdd PROPERTY(migration_chance);
		ConditionalWeightFactorAdd PROPERTY(colonialmigration_chance);
		ConditionalWeightFactorAdd PROPERTY(emigration_chance);
		ConditionalWeightFactorAdd PROPERTY(assimilation_chance);
		ConditionalWeightFactorAdd PROPERTY(conversion_chance);

		// Used if a PopType's strata is invalid or missing (in the base game invalid stratas default to "middle",
		// while missing stratas cause a crash upon starting a game).
		Strata* default_strata = nullptr;

		pop_sprite_t PROPERTY(slave_sprite);
		pop_sprite_t PROPERTY(administrative_sprite);

		CultureManager PROPERTY_REF(culture_manager);
		ReligionManager PROPERTY_REF(religion_manager);

	public:
		PopManager();

		bool add_strata(std::string_view identifier);

		bool add_pop_type(
			std::string_view identifier,
			colour_t new_colour,
			Strata* strata,
			pop_sprite_t sprite,
			memory::fixed_point_map_t<GoodDefinition const*>&& life_needs,
			memory::fixed_point_map_t<GoodDefinition const*>&& everyday_needs,
			memory::fixed_point_map_t<GoodDefinition const*>&& luxury_needs,
			PopType::income_type_t life_needs_income_types,
			PopType::income_type_t everyday_needs_income_types,
			PopType::income_type_t luxury_needs_income_types,
			ast::NodeCPtr rebel_units,
			pop_size_t max_size,
			pop_size_t merge_max_size,
			bool state_capital_only,
			bool demote_migrant,
			bool is_artisan,
			bool allowed_to_vote,
			bool is_slave,
			bool can_be_recruited,
			bool can_reduce_consciousness,
			bool administrative_efficiency,
			bool can_invest,
			bool factory,
			bool can_work_factory,
			bool unemployment,
			fixed_point_t research_points,
			fixed_point_t leadership_points,
			fixed_point_t research_leadership_optimum,
			fixed_point_t state_administration_multiplier,
			ast::NodeCPtr equivalent,
			ConditionalWeightFactorMul&& country_migration_target,
			ConditionalWeightFactorMul&& migration_target,
			ast::NodeCPtr promote_to_node,
			PopType::ideology_weight_map_t&& ideologies,
			ast::NodeCPtr issues_node
		);

		bool setup_stratas();

		void reserve_pop_types_and_delayed_nodes(size_t size);

		bool load_pop_type_file(
			std::string_view filestem, GoodDefinitionManager const& good_definition_manager,
			IdeologyManager const& ideology_manager, ast::NodeCPtr root
		);
		bool load_delayed_parse_pop_type_data(UnitTypeManager const& unit_type_manager, IssueManager const& issue_manager);

		bool load_pop_type_chances_file(ast::NodeCPtr root);

		bool load_pop_bases_into_vector(
			RebelManager const& rebel_manager, memory::vector<PopBase>& vec, PopType const& type, ast::NodeCPtr pop_node,
			bool *non_integer_size
		) const;

		bool generate_modifiers(ModifierManager& modifier_manager) const;

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}