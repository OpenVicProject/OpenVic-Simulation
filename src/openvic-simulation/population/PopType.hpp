#pragma once

#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/population/PopSize.hpp"
#include "openvic-simulation/types/PopSprite.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct BaseIssue;
	struct GoodDefinition;
	struct Ideology;
	struct PopManager;
	struct PopType;
	struct UnitType;

	struct Strata : HasIdentifier, HasIndex<Strata, strata_index_t> {
		friend struct PopManager;

	private:
		memory::vector<PopType const*> SPAN_PROPERTY(pop_types);

	public:
		Strata(std::string_view new_identifier, index_t new_index);
		Strata(Strata&&) = default;
	};

	/* REQUIREMENTS:
	 * POP-15, POP-16, POP-17, POP-26
	 */
	struct PopType : HasIdentifierAndColour, HasIndex<PopType, pop_type_index_t> {
		friend struct PopManager;

		/* This is a bitfield - PopTypes can have up to one of each income source for each need category. */
		enum struct income_type_t : uint8_t {
			NO_INCOME_TYPE  = 0,
			ADMINISTRATION  = 1 << 0,
			EDUCATION       = 1 << 1,
			MILITARY        = 1 << 2
		};

		using rebel_units_t = fixed_point_map_t<UnitType const*>;
		using poptype_weight_map_t = IndexedFlatMap<PopType, ConditionalWeightFactorAdd>;
		using ideology_weight_map_t = IndexedFlatMap<Ideology, ConditionalWeightFactorMul>;
		using issue_weight_map_t = ordered_map<BaseIssue const*, ConditionalWeightFactorMul>;

	private:
		fixed_point_map_t<GoodDefinition const*> PROPERTY(life_needs);
		fixed_point_map_t<GoodDefinition const*> PROPERTY(everyday_needs);
		fixed_point_map_t<GoodDefinition const*> PROPERTY(luxury_needs);
		income_type_t PROPERTY(life_needs_income_types);
		income_type_t PROPERTY(everyday_needs_income_types);
		income_type_t PROPERTY(luxury_needs_income_types);
		rebel_units_t PROPERTY(rebel_units);
		PopType const* PROPERTY(equivalent);

		ConditionalWeightFactorMul PROPERTY(country_migration_target); /* Scope - country, THIS - pop */
		ConditionalWeightFactorMul PROPERTY(migration_target); /* Scope - province, THIS - pop */
		poptype_weight_map_t PROPERTY(promote_to); /* Scope - pop */
		ideology_weight_map_t PROPERTY(ideologies); /* Scope - pop */
		issue_weight_map_t PROPERTY(issues); /* Scope - province, THIS - country (?) */

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		Strata const& strata;
		const pop_sprite_t sprite;
		const pop_size_t max_size;
		const pop_size_t merge_max_size;
		const bool state_capital_only;
		const bool demote_migrant;
		const bool is_artisan;
		const bool allowed_to_vote;
		const bool is_slave;
		const bool can_be_recruited;
		const bool can_reduce_consciousness;
		const bool is_administrator;
		const bool can_invest;
		const bool factory;
		const bool can_work_factory;
		const bool can_be_unemployed;
		const fixed_point_t research_points;
		const fixed_point_t leadership_points;
		const fixed_point_t research_leadership_optimum;
		const fixed_point_t state_administration_multiplier;

		PopType(
			std::string_view new_identifier,
			colour_t new_colour,
			index_t new_index,
			Strata const& new_strata,
			pop_sprite_t new_sprite,
			fixed_point_map_t<GoodDefinition const*>&& new_life_needs,
			fixed_point_map_t<GoodDefinition const*>&& new_everyday_needs,
			fixed_point_map_t<GoodDefinition const*>&& new_luxury_needs,
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
			bool new_is_administrator,
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
#undef BUILD_STRING
		if (!type_found) {
			stream << "INVALID INCOME TYPE";
		}
		return stream << ']';
	}
}