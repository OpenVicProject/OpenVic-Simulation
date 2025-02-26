#pragma once

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/pop/Culture.hpp"
#include "openvic-simulation/pop/Religion.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/PopSize.hpp"

namespace OpenVic {
	struct PopManager;
	struct PopType;
	struct UnitType;
	struct UnitTypeManager;
	struct RebelManager;
	struct Ideology;
	struct IdeologyManager;
	struct Issue;
	struct IssueManager;
	struct PopBase;

	struct Strata : HasIdentifier, HasIndex<> {
		friend struct PopManager;

	private:
		std::vector<PopType const*> PROPERTY(pop_types);

		Strata(std::string_view new_identifier, index_t new_index);

	public:
		Strata(Strata&&) = default;
	};

	/* REQUIREMENTS:
	 * POP-15, POP-16, POP-17, POP-26
	 */
	struct PopType : HasIdentifierAndColour, HasIndex<> {
		friend struct PopManager;

		/* This is a bitfield - PopTypes can have up to one of each income source for each need category. */
		enum struct income_type_t : uint8_t {
			NO_INCOME_TYPE  = 0,
			ADMINISTRATION  = 1 << 0,
			EDUCATION       = 1 << 1,
			MILITARY        = 1 << 2,
			REFORMS         = 1 << 3,
			MAX_INCOME_TYPE = (1 << 4) - 1
		};

		using sprite_t = uint8_t;
		using rebel_units_t = fixed_point_map_t<UnitType const*>;
		using poptype_weight_map_t = IndexedMap<PopType, ConditionalWeightFactorAdd>;
		using ideology_weight_map_t = IndexedMap<Ideology, ConditionalWeightFactorMul>;
		using issue_weight_map_t = ordered_map<Issue const*, ConditionalWeightFactorMul>;

	private:
		Strata const& PROPERTY(strata);
		const sprite_t PROPERTY(sprite);
		GoodDefinition::good_definition_map_t PROPERTY(life_needs);
		GoodDefinition::good_definition_map_t PROPERTY(everyday_needs);
		GoodDefinition::good_definition_map_t PROPERTY(luxury_needs);
		income_type_t PROPERTY(life_needs_income_types);
		income_type_t PROPERTY(everyday_needs_income_types);
		income_type_t PROPERTY(luxury_needs_income_types);
		rebel_units_t PROPERTY(rebel_units);
		const pop_size_t PROPERTY(max_size);
		const pop_size_t PROPERTY(merge_max_size);
		const bool PROPERTY(state_capital_only);
		const bool PROPERTY(demote_migrant);
		const bool PROPERTY(is_artisan);
		const bool PROPERTY(allowed_to_vote);
		const bool PROPERTY(is_slave);
		const bool PROPERTY(can_be_recruited);
		const bool PROPERTY(can_reduce_consciousness);
		const bool PROPERTY(administrative_efficiency);
		const bool PROPERTY(can_invest);
		const bool PROPERTY(factory);
		const bool PROPERTY(can_work_factory);
		const bool PROPERTY(can_be_unemployed);
		const fixed_point_t PROPERTY(research_points);
		const fixed_point_t PROPERTY(leadership_points);
		const fixed_point_t PROPERTY(research_leadership_optimum);
		const fixed_point_t PROPERTY(state_administration_multiplier);
		PopType const* PROPERTY(equivalent);

		ConditionalWeightFactorMul PROPERTY(country_migration_target); /* Scope - country, THIS - pop */
		ConditionalWeightFactorMul PROPERTY(migration_target); /* Scope - province, THIS - pop */
		poptype_weight_map_t PROPERTY(promote_to); /* Scope - pop */
		ideology_weight_map_t PROPERTY(ideologies); /* Scope - pop */
		issue_weight_map_t PROPERTY(issues); /* Scope - province, THIS - country (?) */

		PopType(
			std::string_view new_identifier,
			colour_t new_colour,
			index_t new_index,
			Strata const& new_strata,
			sprite_t new_sprite,
			GoodDefinition::good_definition_map_t&& new_life_needs,
			GoodDefinition::good_definition_map_t&& new_everyday_needs,
			GoodDefinition::good_definition_map_t&& new_luxury_needs,
			income_type_t new_life_needs_income_types,
			income_type_t new_everyday_needs_income_types,
			income_type_t new_luxury_needs_income_types,
			rebel_units_t&& new_rebel_units,
			pop_size_t new_max_size,
			pop_size_t new_merge_max_size,
			bool new_state_capital_only,
			bool new_demote_migrant,
			bool new_is_artisan,
			bool new_allowed_to_vote,
			bool new_is_slave,
			bool new_can_be_recruited,
			bool new_can_reduce_consciousness,
			bool new_administrative_efficiency,
			bool new_can_invest,
			bool new_factory,
			bool new_can_work_factory,
			bool new_can_be_unemployed,
			fixed_point_t new_research_points,
			fixed_point_t new_leadership_points,
			fixed_point_t new_research_leadership_optimum,
			fixed_point_t new_state_administration_multiplier,
			PopType const* new_equivalent,
			ConditionalWeightFactorMul&& new_country_migration_target,
			ConditionalWeightFactorMul&& new_migration_target,
			poptype_weight_map_t&& new_promote_to,
			ideology_weight_map_t&& new_ideologies,
			issue_weight_map_t&& new_issues
		);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		PopType(PopType&&) = default;

		constexpr bool has_income_type(income_type_t income_type) const;
	};

	template<> struct enable_bitfield<PopType::income_type_t> : std::true_type {};

	constexpr bool PopType::has_income_type(income_type_t income_type) const {
		return (life_needs_income_types & income_type) == income_type
			|| (everyday_needs_income_types & income_type) == income_type
			|| (luxury_needs_income_types & income_type) == income_type;
	}

	/* This returns true if at least one income type is shared by both arguments. */
	inline constexpr bool share_income_type(PopType::income_type_t lhs, PopType::income_type_t rhs) {
		return (lhs & rhs) != PopType::income_type_t::NO_INCOME_TYPE;
	}

	inline std::ostream& operator<<(std::ostream& stream, PopType::income_type_t income_type) {
		using enum PopType::income_type_t;
		if (income_type == NO_INCOME_TYPE) {
			return stream << "[NO_INCOME_TYPE]";
		}
		bool type_found = false;
		stream << '[';
#define BUILD_STRING(entry) \
	if (share_income_type(income_type, entry)) { \
		if (type_found) { \
			stream << " | "; \
		} else { \
			type_found = true; \
		} \
		stream << #entry; \
	}
		BUILD_STRING(ADMINISTRATION);
		BUILD_STRING(EDUCATION);
		BUILD_STRING(MILITARY);
		BUILD_STRING(REFORMS);
#undef BUILD_STRING
		if (!type_found) {
			stream << "INVALID INCOME TYPE";
		}
		return stream << ']';
	}

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
		std::vector<std::tuple<ast::NodeCPtr, ast::NodeCPtr, ast::NodeCPtr, ast::NodeCPtr>> delayed_parse_nodes;

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

		PopType::sprite_t PROPERTY(slave_sprite);
		PopType::sprite_t PROPERTY(administrative_sprite);

		CultureManager PROPERTY_REF(culture_manager);
		ReligionManager PROPERTY_REF(religion_manager);

	public:
		PopManager();

		bool add_strata(std::string_view identifier);

		bool add_pop_type(
			std::string_view identifier,
			colour_t new_colour,
			Strata* strata,
			PopType::sprite_t sprite,
			GoodDefinition::good_definition_map_t&& life_needs,
			GoodDefinition::good_definition_map_t&& everyday_needs,
			GoodDefinition::good_definition_map_t&& luxury_needs,
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
			RebelManager const& rebel_manager, std::vector<PopBase>& vec, PopType const& type, ast::NodeCPtr pop_node,
			bool *non_integer_size
		) const;

		bool generate_modifiers(ModifierManager& modifier_manager) const;

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}