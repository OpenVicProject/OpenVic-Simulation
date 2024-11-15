#pragma once

#include <limits>
#include <ostream>
#include <tuple>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/pop/Culture.hpp"
#include "openvic-simulation/pop/Religion.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/EnumBitfield.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"

namespace OpenVic {

	struct PopManager;
	struct PopType;
	struct UnitType;
	struct UnitTypeManager;
	struct RebelType;
	struct RebelManager;
	struct Ideology;
	struct IdeologyManager;
	struct Issue;
	struct IssueManager;
	struct ProvinceInstance;
	struct CountryParty;
	struct DefineManager;
	struct CountryInstance;

	struct PopBase {
		friend struct PopManager;

		using pop_size_t = int32_t;

	protected:
		PopType const* PROPERTY_ACCESS(type, protected);
		Culture const& PROPERTY_ACCESS(culture, protected);
		Religion const& PROPERTY_ACCESS(religion, protected);
		pop_size_t PROPERTY_ACCESS(size, protected);
		fixed_point_t PROPERTY_ACCESS(militancy, protected);
		fixed_point_t PROPERTY_ACCESS(consciousness, protected);
		RebelType const* PROPERTY_ACCESS(rebel_type, protected);

		PopBase(
			PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size,
			fixed_point_t new_militancy, fixed_point_t new_consciousness, RebelType const* new_rebel_type
		);
	};

	#define DO_FOR_ALL_TYPES_OF_POP_INCOME(F)\
		F(rgo_owner_income)\
		F(rgo_worker_income)\
		F(artisanal_income)\
		F(factory_worker_income)\
		F(factory_owner_income)\
		F(unemployment_subsidies)\
		F(pensions)\
		F(government_salary_administration)\
		F(government_salary_education)\
		F(government_salary_military)\
		F(event_and_decision_income)\
		F(loan_interest_payments)
	
	#define DECLARE_POP_INCOME_STORES(income_type)\
		fixed_point_t PROPERTY(income_type); 
	
	#define DECLARE_POP_INCOME_STORE_FUNCTIONS(name)\
		void add_##name(const fixed_point_t pop_income);

	/* REQUIREMENTS:
	 * POP-18, POP-19, POP-20, POP-21, POP-34, POP-35, POP-36, POP-37
	 */
	struct Pop : PopBase {
		friend struct ProvinceInstance;

		static constexpr pop_size_t MAX_SIZE = std::numeric_limits<pop_size_t>::max();

	private:
		ProvinceInstance const* PROPERTY(location);

		/* Last day's size change by source. */
		pop_size_t PROPERTY(total_change);
		pop_size_t PROPERTY(num_grown);
		pop_size_t PROPERTY(num_promoted); // TODO - detailed promotion/demotion info (what to)
		pop_size_t PROPERTY(num_demoted);
		pop_size_t PROPERTY(num_migrated_internal); // TODO - detailed migration info (where to)
		pop_size_t PROPERTY(num_migrated_external);
		pop_size_t PROPERTY(num_migrated_colonial);

		fixed_point_t PROPERTY(literacy);

		IndexedMap<Ideology, fixed_point_t> PROPERTY(ideologies);
		fixed_point_map_t<Issue const*> PROPERTY(issues);
		IndexedMap<CountryParty, fixed_point_t> PROPERTY(votes);

		fixed_point_t PROPERTY(unemployment);
		fixed_point_t PROPERTY(cash);
		fixed_point_t PROPERTY(income);
		fixed_point_t PROPERTY(expenses);
		fixed_point_t PROPERTY(savings);
		fixed_point_t PROPERTY(life_needs_fulfilled);
		fixed_point_t PROPERTY(everyday_needs_fulfilled);
		fixed_point_t PROPERTY(luxury_needs_fulfilled);

		DO_FOR_ALL_TYPES_OF_POP_INCOME(DECLARE_POP_INCOME_STORES);
		#undef DECLARE_POP_INCOME_STORES
		
		size_t PROPERTY(max_supported_regiments);

		Pop(PopBase const& pop_base, decltype(ideologies)::keys_t const& ideology_keys);

	public:
		Pop(Pop const&) = delete;
		Pop(Pop&&) = default;
		Pop& operator=(Pop const&) = delete;
		Pop& operator=(Pop&&) = delete;

		void setup_pop_test_values(IssueManager const& issue_manager);
		bool convert_to_equivalent();

		void set_location(ProvinceInstance const& new_location);

		void update_gamestate(
			DefineManager const& define_manager, CountryInstance const* owner,
			const fixed_point_t pop_size_per_regiment_multiplier
		);

		DO_FOR_ALL_TYPES_OF_POP_INCOME(DECLARE_POP_INCOME_STORE_FUNCTIONS)
		#undef DECLARE_POP_INCOME_STORE_FUNCTIONS
		void clear_all_income();

	};

	struct Strata : HasIdentifier {
		friend struct PopManager;

	private:
		Strata(std::string_view new_identifier);

	public:
		Strata(Strata&&) = default;
	};

	/* REQUIREMENTS:
	 * POP-15, POP-16, POP-17, POP-26
	 */
	struct PopType : HasIdentifierAndColour {
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
		using poptype_weight_map_t = ordered_map<PopType const*, ConditionalWeight>;
		using ideology_weight_map_t = ordered_map<Ideology const*, ConditionalWeight>;
		using issue_weight_map_t = ordered_map<Issue const*, ConditionalWeight>;

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
		const Pop::pop_size_t PROPERTY(max_size);
		const Pop::pop_size_t PROPERTY(merge_max_size);
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
		const bool PROPERTY(unemployment);
		const fixed_point_t PROPERTY(research_points);
		const fixed_point_t PROPERTY(leadership_points);
		const fixed_point_t PROPERTY(research_leadership_optimum);
		const fixed_point_t PROPERTY(state_administration_multiplier);
		PopType const* PROPERTY(equivalent);

		ConditionalWeight PROPERTY(country_migration_target); /* Scope - country, THIS - pop */
		ConditionalWeight PROPERTY(migration_target); /* Scope - province, THIS - pop */
		poptype_weight_map_t PROPERTY(promote_to); /* Scope - pop */
		ideology_weight_map_t PROPERTY(ideologies); /* Scope - pop */
		issue_weight_map_t PROPERTY(issues); /* Scope - province, THIS - country (?) */

		PopType(
			std::string_view new_identifier,
			colour_t new_colour,
			Strata const& new_strata,
			sprite_t new_sprite,
			GoodDefinition::good_definition_map_t&& new_life_needs,
			GoodDefinition::good_definition_map_t&& new_everyday_needs,
			GoodDefinition::good_definition_map_t&& new_luxury_needs,
			income_type_t new_life_needs_income_types,
			income_type_t new_everyday_needs_income_types,
			income_type_t new_luxury_needs_income_types,
			rebel_units_t&& new_rebel_units,
			Pop::pop_size_t new_max_size,
			Pop::pop_size_t new_merge_max_size,
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
			bool new_unemployment,
			fixed_point_t new_research_points,
			fixed_point_t new_leadership_points,
			fixed_point_t new_research_leadership_optimum,
			fixed_point_t new_state_administration_multiplier,
			PopType const* new_equivalent,
			ConditionalWeight&& new_country_migration_target,
			ConditionalWeight&& new_migration_target,
			poptype_weight_map_t&& new_promote_to,
			ideology_weight_map_t&& new_ideologies,
			issue_weight_map_t&& new_issues
		);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		PopType(PopType&&) = default;
	};

	template<> struct enable_bitfield<PopType::income_type_t> : std::true_type {};

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

		ConditionalWeight PROPERTY(promotion_chance);
		ConditionalWeight PROPERTY(demotion_chance);
		ConditionalWeight PROPERTY(migration_chance);
		ConditionalWeight PROPERTY(colonialmigration_chance);
		ConditionalWeight PROPERTY(emigration_chance);
		ConditionalWeight PROPERTY(assimilation_chance);
		ConditionalWeight PROPERTY(conversion_chance);

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
			Strata const* strata,
			PopType::sprite_t sprite,
			GoodDefinition::good_definition_map_t&& life_needs,
			GoodDefinition::good_definition_map_t&& everyday_needs,
			GoodDefinition::good_definition_map_t&& luxury_needs,
			PopType::income_type_t life_needs_income_types,
			PopType::income_type_t everyday_needs_income_types,
			PopType::income_type_t luxury_needs_income_types,
			ast::NodeCPtr rebel_units,
			Pop::pop_size_t max_size,
			Pop::pop_size_t merge_max_size,
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
			ConditionalWeight&& country_migration_target,
			ConditionalWeight&& migration_target,
			ast::NodeCPtr promote_to_node,
			PopType::ideology_weight_map_t&& ideologies,
			ast::NodeCPtr issues_node
		);

		void reserve_all_pop_types(size_t size);
		void lock_all_pop_types();

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
#ifndef KEEP_DO_FOR_ALL_TYPES_OF_INCOME 
	#undef DO_FOR_ALL_TYPES_OF_POP_INCOME
#endif
