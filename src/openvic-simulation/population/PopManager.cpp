#include "PopManager.hpp"

#include <fmt/format.h>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/IssueManager.hpp"
#include "openvic-simulation/politics/Rebel.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"
#include "openvic-simulation/utility/FormatValidate.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

PopManager::PopManager()
  : slave_sprite { 0 }, administrative_sprite { 0 },
	promotion_chance { scope_type_t::POP, scope_type_t::POP, scope_type_t::NO_SCOPE },
	demotion_chance { scope_type_t::POP, scope_type_t::POP, scope_type_t::NO_SCOPE },
	migration_chance { scope_type_t::POP, scope_type_t::POP, scope_type_t::NO_SCOPE },
	colonialmigration_chance { scope_type_t::POP, scope_type_t::POP, scope_type_t::NO_SCOPE },
	emigration_chance { scope_type_t::POP, scope_type_t::POP, scope_type_t::NO_SCOPE },
	assimilation_chance { scope_type_t::POP, scope_type_t::POP, scope_type_t::NO_SCOPE },
	conversion_chance { scope_type_t::POP, scope_type_t::POP, scope_type_t::NO_SCOPE } {}

bool PopManager::setup_stratas() {
	if (stratas_are_locked()) {
		spdlog::error_s("Failed to set up stratas - already locked!");
		return false;
	}

	static constexpr std::array<std::string_view, 3> STRATA_IDENTIFIERS {
		"poor", "middle", "rich"
	};

	reserve_more_stratas(STRATA_IDENTIFIERS.size());

	bool ret = true;

	for (std::string_view identifier : STRATA_IDENTIFIERS) {
		ret &= add_strata(identifier);
	}

	lock_stratas();

	// Default strata is middle
	static constexpr size_t DEFAULT_STRATA_INDEX = 1;
	default_strata = stratas.get_item_by_identifier(STRATA_IDENTIFIERS[DEFAULT_STRATA_INDEX]);

	if (!ret) {
		spdlog::error_s("Error while setting up stratas!");
	}

	return ret;
}

bool PopManager::add_strata(std::string_view identifier) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid strata identifier - empty!");
		return false;
	}
	return stratas.emplace_item(
		identifier,
		identifier, get_strata_count()
	);
}

bool PopManager::add_pop_type(
	std::string_view identifier,
	colour_t colour,
	Strata* strata,
	pop_sprite_t sprite,
	fixed_point_map_t<GoodDefinition const*>&& life_needs,
	fixed_point_map_t<GoodDefinition const*>&& everyday_needs,
	fixed_point_map_t<GoodDefinition const*>&& luxury_needs,
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
) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid pop type identifier - empty!");
		return false;
	}
	if (strata == nullptr) {
		spdlog::error_s("Invalid pop type strata for {} - null!", identifier);
		return false;
	}
	if (sprite <= 0) {
		spdlog::error_s(
			"Invalid pop type sprite index for {}: {} (must be positive)",
			identifier, sprite
		);
		return false;
	}
	if (max_size <= 0) {
		spdlog::error_s(
			"Invalid pop type max size for {}: {} (must be positive)",
			identifier, max_size
		);
		return false;
	}
	if (merge_max_size <= 0) {
		spdlog::error_s(
			"Invalid pop type merge max size for {}: {} (must be positive)",
			identifier, merge_max_size
		);
		return false;
	}

	if (research_leadership_optimum < 0) {
		spdlog::error_s(
			"Invalid pop type research/leadership optimum for {}: {} (cannot be negative)",
			identifier, research_leadership_optimum
		);
		return false;
	}
	if ((research_points != 0 || leadership_points != 0) != (research_leadership_optimum > 0)) {
		spdlog::error_s(
			"Invalid pop type research/leadership points and optimum for {}: research = {}, leadership = {}, optimum = {} "
			"(optimum is positive if and only if at least one of research and leadership is non-zero)",
			identifier, research_points, leadership_points, research_leadership_optimum
		);
		return false;
	}

	if (OV_unlikely(!pop_types.emplace_item(
		identifier,
		identifier,
		colour,
		get_pop_type_count(),
		*strata,
		sprite,
		std::move(life_needs),
		std::move(everyday_needs),
		std::move(luxury_needs),
		life_needs_income_types,
		everyday_needs_income_types,
		luxury_needs_income_types,
		PopType::rebel_units_t{},
		max_size,
		merge_max_size,
		state_capital_only,
		demote_migrant,
		is_artisan,
		allowed_to_vote,
		is_slave,
		can_be_recruited,
		can_reduce_consciousness,
		administrative_efficiency,
		can_invest,
		factory,
		can_work_factory,
		unemployment,
		research_points,
		leadership_points,
		research_leadership_optimum,
		state_administration_multiplier,
		static_cast<PopType const*>(nullptr),
		std::move(country_migration_target),
		std::move(migration_target),
		PopType::poptype_weight_map_t { PopType::poptype_weight_map_t::create_empty() },
		std::move(ideologies),
		PopType::issue_weight_map_t{}
	))) {
		return false;
	}

	delayed_parse_nodes.emplace_back(rebel_units, equivalent, promote_to_node, issues_node);

	strata->pop_types.push_back(&pop_types.back());

	if (slave_sprite <= 0 && is_slave) {
		/* Set slave sprite to that of the first is_slave pop type we find. */
		slave_sprite = sprite;
	}

	if (administrative_sprite <= 0 && administrative_efficiency) {
		/* Set administrative sprite to that of the first administrative_efficiency pop type we find. */
		administrative_sprite = sprite;
	}

	return true;
}

void PopManager::reserve_pop_types_and_delayed_nodes(size_t size) {
	if (pop_types_are_locked()) {
		spdlog::error_s("Failed to reserve space for {} pop types in PopManager - already locked!", size);
	} else {
		reserve_more_pop_types(size);
		reserve_more(delayed_parse_nodes, size);
	}
}

static NodeCallback auto expect_needs_income(PopType::income_type_t& types) {
	using enum PopType::income_type_t;
	static const string_map_t<PopType::income_type_t> income_type_map {
		{ "administration", ADMINISTRATION },
		{ "education", EDUCATION },
		{ "military", MILITARY }
	};

	constexpr bool is_warning = true;
	return expect_dictionary_keys_and_default_map(
		map_key_value_ignore_invalid_callback<template_key_map_t<StringMapCaseSensitive>>,
		"type", ONE_OR_MORE, expect_identifier(expect_mapped_string(
			income_type_map,
			[&types](PopType::income_type_t type) -> bool {
				if (OV_unlikely(share_income_type(types, type))) {
					spdlog::warn_s("Duplicate pop income type {} is treated as single income.", fmt::underlying(type));
				} else {
					types |= type;
				}
				return true;
			},
			is_warning
		))
	);
}

/* REQUIREMENTS:
 * POP-3, POP-4, POP-5, POP-6, POP-7, POP-8, POP-9, POP-10, POP-11, POP-12, POP-13, POP-14
 */
bool PopManager::load_pop_type_file(
	std::string_view filestem, GoodDefinitionManager const& good_definition_manager, IdeologyManager const& ideology_manager,
	ast::NodeCPtr root
) {
	spdlog::scope scope { fmt::format("poptypes/{}.txt", filestem) };
	using enum scope_type_t;
	using enum PopType::income_type_t;

	colour_t colour = colour_t::null();
	Strata* strata = default_strata;
	pop_sprite_t sprite = 0;
	fixed_point_map_t<GoodDefinition const*> life_needs, everyday_needs, luxury_needs;
	PopType::income_type_t life_needs_income_types = NO_INCOME_TYPE, everyday_needs_income_types = NO_INCOME_TYPE,
		luxury_needs_income_types = NO_INCOME_TYPE;
	ast::NodeCPtr rebel_units = nullptr;
	pop_size_t max_size = Pop::MAX_SIZE, merge_max_size = Pop::MAX_SIZE;
	bool state_capital_only = false, demote_migrant = false, is_artisan = false, allowed_to_vote = true, is_slave = false,
		can_be_recruited = false, can_reduce_consciousness = false, administrative_efficiency = false, can_invest = false,
		factory = false, can_work_factory = false, unemployment = false;
	fixed_point_t research_points = 0, leadership_points = 0, research_leadership_optimum = 0,
		state_administration_multiplier = 0;
	ast::NodeCPtr equivalent = nullptr;
	ConditionalWeightFactorMul country_migration_target { COUNTRY, POP, NO_SCOPE };
	ConditionalWeightFactorMul migration_target { PROVINCE, POP, NO_SCOPE };
	ast::NodeCPtr promote_to_node = nullptr;
	PopType::ideology_weight_map_t ideologies { ideology_manager.get_ideologies() };
	ast::NodeCPtr issues_node = nullptr;

	bool ret = expect_dictionary_keys(
		"sprite", ONE_EXACTLY, expect_uint(assign_variable_callback(sprite)),
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
		"is_artisan", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_artisan)),
		"max_size", ZERO_OR_ONE, expect_uint(assign_variable_callback(max_size)),
		"merge_max_size", ZERO_OR_ONE, expect_uint(assign_variable_callback(merge_max_size)),
		"strata", ONE_EXACTLY, stratas.expect_item_identifier(assign_variable_callback_pointer(strata)),
		"state_capital_only", ZERO_OR_ONE, expect_bool(assign_variable_callback(state_capital_only)),
		"research_points", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(research_points)),
		"research_optimum", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(research_leadership_optimum)),
		"rebel", ZERO_OR_ONE, assign_variable_callback(rebel_units),
		"equivalent", ZERO_OR_ONE, assign_variable_callback(equivalent),
		"leadership", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(leadership_points)),
		"allowed_to_vote", ZERO_OR_ONE, expect_bool(assign_variable_callback(allowed_to_vote)),
		"is_slave", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_slave)),
		"can_be_recruited", ZERO_OR_ONE, expect_bool(assign_variable_callback(can_be_recruited)),
		"can_reduce_consciousness", ZERO_OR_ONE, expect_bool(assign_variable_callback(can_reduce_consciousness)),
		"life_needs_income", ZERO_OR_ONE, expect_needs_income(life_needs_income_types),
		"everyday_needs_income", ZERO_OR_ONE, expect_needs_income(everyday_needs_income_types),
		"luxury_needs_income", ZERO_OR_ONE, expect_needs_income(luxury_needs_income_types),
		"luxury_needs", ZERO_OR_ONE,
			good_definition_manager.expect_good_definition_decimal_map(move_variable_callback(luxury_needs)),
		"everyday_needs", ZERO_OR_ONE,
			good_definition_manager.expect_good_definition_decimal_map(move_variable_callback(everyday_needs)),
		"life_needs", ZERO_OR_ONE,
			good_definition_manager.expect_good_definition_decimal_map(move_variable_callback(life_needs)),
		"country_migration_target", ZERO_OR_ONE, country_migration_target.expect_conditional_weight(),
		"migration_target", ZERO_OR_ONE, migration_target.expect_conditional_weight(),
		"promote_to", ZERO_OR_ONE, assign_variable_callback(promote_to_node),
		"ideologies", ZERO_OR_ONE, ideology_manager.expect_ideology_dictionary(
			[&filestem, &ideologies](Ideology const& ideology, ast::NodeCPtr node) -> bool {
				ConditionalWeightFactorMul weight { POP, POP, NO_SCOPE };

				bool ret = weight.expect_conditional_weight()(node);

				ret &= map_callback(ideologies, &ideology)(std::move(weight));

				return ret;
			}
		),
		"issues", ZERO_OR_ONE, assign_variable_callback(issues_node),
		"demote_migrant", ZERO_OR_ONE, expect_bool(assign_variable_callback(demote_migrant)),
		"administrative_efficiency", ZERO_OR_ONE, expect_bool(assign_variable_callback(administrative_efficiency)),
		"tax_eff", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(state_administration_multiplier)),
		"can_build", ZERO_OR_ONE, expect_bool(assign_variable_callback(can_invest)),
		"factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(factory)), // TODO - work out what this does
		"workplace_input", ZERO_OR_ONE, success_callback, // TODO - work out what these do
		"workplace_output", ZERO_OR_ONE, success_callback,
		"starter_share", ZERO_OR_ONE, success_callback,
		"can_work_factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(can_work_factory)),
		"unemployment", ZERO_OR_ONE, expect_bool(assign_variable_callback(unemployment))
	)(root);

	ret &= add_pop_type(
		filestem,
		colour,
		strata,
		sprite,
		std::move(life_needs),
		std::move(everyday_needs),
		std::move(luxury_needs),
		life_needs_income_types,
		everyday_needs_income_types,
		luxury_needs_income_types,
		rebel_units,
		max_size,
		merge_max_size,
		state_capital_only,
		demote_migrant,
		is_artisan,
		allowed_to_vote,
		is_slave,
		can_be_recruited,
		can_reduce_consciousness,
		administrative_efficiency,
		can_invest,
		factory,
		can_work_factory,
		unemployment,
		research_points,
		leadership_points,
		research_leadership_optimum,
		state_administration_multiplier,
		equivalent,
		std::move(country_migration_target),
		std::move(migration_target),
		promote_to_node,
		std::move(ideologies),
		issues_node
	);
	return ret;
}

bool PopManager::load_delayed_parse_pop_type_data(
	UnitTypeManager const& unit_type_manager, IssueManager const& issue_manager
) {
	using enum scope_type_t;

	bool ret = true;
	for (size_t index = 0; index < delayed_parse_nodes.size(); ++index) {
		const auto [rebel_units, equivalent, promote_to_node, issues_node] = delayed_parse_nodes[index];
		PopType* pop_type = pop_types.get_item_by_index(index);

		pop_type->promote_to = std::move(decltype(PopType::promote_to){get_pop_types()});

		if (rebel_units != nullptr && !unit_type_manager.expect_unit_type_decimal_map(
			move_variable_callback(pop_type->rebel_units)
		)(rebel_units)) {
			spdlog::error_s("Errors parsing rebel unit distribution for pop type {}!", ovfmt::validate(pop_type));
			ret = false;
		}

		if (equivalent != nullptr && !expect_pop_type_identifier(
			assign_variable_callback_pointer(pop_type->equivalent)
		)(equivalent)) {
			spdlog::error_s("Errors parsing equivalent pop type for pop type {}!", ovfmt::validate(pop_type));
			ret = false;
		}

		if (promote_to_node != nullptr && !expect_pop_type_dictionary(
			[pop_type](PopType const& type, ast::NodeCPtr node) -> bool {
				if (pop_type && type == *pop_type) {
					spdlog::error_s("Pop type {} cannot have promotion weight to itself!", type);
					return false;
				}
				ConditionalWeightFactorAdd weight { POP, POP, NO_SCOPE };
				bool ret = weight.expect_conditional_weight()(node);
				ret &= map_callback(pop_type->promote_to, &type)(std::move(weight));
				return ret;
			}
		)(promote_to_node)) {
			spdlog::error_s("Errors parsing promotion weights for pop type {}!", ovfmt::validate(pop_type));
			ret = false;
		}

		if (issues_node != nullptr && !expect_dictionary_reserve_length(
			pop_type->issues,
			[pop_type, &issue_manager](std::string_view key, ast::NodeCPtr node) -> bool {
				BaseIssue const* issue = issue_manager.get_base_issue_by_identifier(key);
				if (issue == nullptr) {
					spdlog::error_s("Invalid issue in pop type {} issue weights: {}", ovfmt::validate(pop_type), key);
					return false;
				}
				ConditionalWeightFactorMul weight { POP, POP, NO_SCOPE };
				bool ret = weight.expect_conditional_weight()(node);
				ret &= map_callback(pop_type->issues, issue)(std::move(weight));
				return ret;
			}
		)(issues_node)) {
			spdlog::error_s("Errors parsing issue weights for pop type {}!", ovfmt::validate(pop_type));
			ret = false;
		}
	}
	delayed_parse_nodes.clear();
	return ret;
}

bool PopManager::load_pop_type_chances_file(ast::NodeCPtr root) {
	return expect_dictionary_keys(
		"promotion_chance", ONE_EXACTLY, promotion_chance.expect_conditional_weight(),
		"demotion_chance", ONE_EXACTLY, demotion_chance.expect_conditional_weight(),
		"migration_chance", ONE_EXACTLY, migration_chance.expect_conditional_weight(),
		"colonialmigration_chance", ONE_EXACTLY, colonialmigration_chance.expect_conditional_weight(),
		"emigration_chance", ONE_EXACTLY, emigration_chance.expect_conditional_weight(),
		"assimilation_chance", ONE_EXACTLY, assimilation_chance.expect_conditional_weight(),
		"conversion_chance", ONE_EXACTLY, conversion_chance.expect_conditional_weight()
	)(root);
}

bool PopManager::load_pop_bases_into_vector(
	RebelManager const& rebel_manager, memory::vector<PopBase>& vec, PopType const& type, ast::NodeCPtr pop_node,
	bool *non_integer_size
) const {
	Culture const* culture = nullptr;
	Religion const* religion = nullptr;
	fixed_point_t size = 0; /* Some genius filled later start dates with non-integer sized pops */
	fixed_point_t militancy = 0, consciousness = 0;
	RebelType const* rebel_type = nullptr;

	constexpr bool do_warn = true;
	bool ret = expect_dictionary_keys(
		"culture", ONE_EXACTLY, culture_manager.expect_culture_identifier(assign_variable_callback_pointer(culture), do_warn),
		"religion", ONE_EXACTLY, religion_manager.expect_religion_identifier(assign_variable_callback_pointer(religion), do_warn),
		"size", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(size)),
		"militancy", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(militancy)),
		"consciousness", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(consciousness)),
		"rebel_type", ZERO_OR_ONE, rebel_manager.expect_rebel_type_identifier(assign_variable_callback_pointer(rebel_type))
	)(pop_node);

	if (non_integer_size != nullptr && !size.is_integer()) {
		*non_integer_size = true;
	}
	if (culture == nullptr || religion == nullptr) {
		spdlog::warn_s("No/invalid culture or religion defined for pop of size {}, ignored!", size);
		return true;
	}

	if (culture != nullptr && religion != nullptr && size >= 1 && size <= std::numeric_limits<pop_size_t>::max()) {
		vec.emplace_back(PopBase { type, *culture, *religion, size.floor<int32_t>(), militancy, consciousness, rebel_type });
	} else {
		spdlog::warn_s(
			"Some pop arguments are invalid: culture = {}, religion = {}, size = {}",
			ovfmt::validate(culture), ovfmt::validate(religion), size
		);
	}
	return ret;
}

bool PopManager::generate_modifiers(ModifierManager& modifier_manager) const {
	using enum ModifierEffect::format_t;
	using enum ModifierEffect::target_t;

	static constexpr bool HAS_NO_EFFECT = true;

	IndexedFlatMap<Strata, ModifierEffectCache::strata_effects_t>& strata_effects =
		modifier_manager.modifier_effect_cache.strata_effects;

	strata_effects = std::move(decltype(ModifierEffectCache::strata_effects){get_stratas()});

	bool ret = true;

	for (Strata const& strata : get_stratas()) {
		const auto strata_modifier = [&modifier_manager, &ret, &strata](
			ModifierEffect const*& effect_cache, std::string_view suffix, ModifierEffect::format_t format,
			bool has_no_effect = false
		) -> void {
			ret &= modifier_manager.register_base_country_modifier_effect(
				effect_cache, StringUtils::append_string_views(strata.get_identifier(), suffix), format, {}, has_no_effect
			);
		};

		ModifierEffectCache::strata_effects_t& this_strata_effects = strata_effects.at(strata);

		strata_modifier(this_strata_effects.income_modifier, "_income_modifier", FORMAT_x100_1DP_PC_POS, HAS_NO_EFFECT);
		strata_modifier(this_strata_effects.vote, "_vote", FORMAT_x100_1DP_PC_POS);

		strata_modifier(this_strata_effects.life_needs, "_life_needs", FORMAT_x100_1DP_PC_NEG);
		strata_modifier(this_strata_effects.everyday_needs, "_everyday_needs", FORMAT_x100_1DP_PC_NEG);
		strata_modifier(this_strata_effects.luxury_needs, "_luxury_needs", FORMAT_x100_1DP_PC_NEG);
	}

	return ret;
}

bool PopManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	for (PopType& pop_type : pop_types.get_items()) {
		ret &= pop_type.parse_scripts(definition_manager);
	}
	ret &= promotion_chance.parse_scripts(definition_manager);
	ret &= demotion_chance.parse_scripts(definition_manager);
	ret &= migration_chance.parse_scripts(definition_manager);
	ret &= colonialmigration_chance.parse_scripts(definition_manager);
	ret &= emigration_chance.parse_scripts(definition_manager);
	ret &= assimilation_chance.parse_scripts(definition_manager);
	ret &= conversion_chance.parse_scripts(definition_manager);
	return ret;
}
