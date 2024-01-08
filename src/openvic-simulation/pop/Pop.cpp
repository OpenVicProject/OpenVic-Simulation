#include "Pop.hpp"

#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"
#include "openvic-simulation/politics/Rebel.hpp"
#include "openvic-simulation/utility/TslHelper.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Pop::Pop(
	PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size,
	fixed_point_t new_militancy, fixed_point_t new_consciousness, RebelType const* new_rebel_type
) : type { new_type }, culture { new_culture }, religion { new_religion }, size { new_size }, num_grown { 0 },
	num_promoted { 0 }, num_demoted { 0 }, num_migrated_internal { 0 }, num_migrated_external { 0 },
	num_migrated_colonial { 0 }, militancy { new_militancy }, consciousness { new_consciousness },
	rebel_type { new_rebel_type } {
	assert(size > 0);
}

Strata::Strata(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

PopType::PopType(
	std::string_view new_identifier, colour_t new_colour, Strata const& new_strata, sprite_t new_sprite,
	Good::good_map_t&& new_life_needs, Good::good_map_t&& new_everyday_needs, Good::good_map_t&& new_luxury_needs,
	rebel_units_t&& new_rebel_units, Pop::pop_size_t new_max_size, Pop::pop_size_t new_merge_max_size,
	bool new_state_capital_only, bool new_demote_migrant, bool new_is_artisan, bool new_allowed_to_vote, bool new_is_slave,
	bool new_can_be_recruited, bool new_can_reduce_consciousness, bool new_administrative_efficiency, bool new_can_build,
	bool new_factory, bool new_can_work_factory, bool new_unemployment, ConditionalWeight&& new_country_migration_target,
	ConditionalWeight&& new_migration_target, poptype_weight_map_t&& new_promote_to, ideology_weight_map_t&& new_ideologies,
	issue_weight_map_t&& new_issues
) : HasIdentifierAndColour { new_identifier, new_colour, false }, strata { new_strata }, sprite { new_sprite },
	life_needs { std::move(new_life_needs) }, everyday_needs { std::move(new_everyday_needs) },
	luxury_needs { std::move(new_luxury_needs) }, rebel_units { std::move(new_rebel_units) }, max_size { new_max_size },
	merge_max_size { new_merge_max_size }, state_capital_only { new_state_capital_only },
	demote_migrant { new_demote_migrant }, is_artisan { new_is_artisan }, allowed_to_vote { new_allowed_to_vote },
	is_slave { new_is_slave }, can_be_recruited { new_can_be_recruited },
	can_reduce_consciousness { new_can_reduce_consciousness }, administrative_efficiency { new_administrative_efficiency },
	can_build { new_can_build }, factory { new_factory }, can_work_factory { new_can_work_factory },
	unemployment { new_unemployment }, country_migration_target { std::move(new_country_migration_target) },
	migration_target { std::move(new_migration_target) }, promote_to { std::move(new_promote_to) },
	ideologies { std::move(new_ideologies) }, issues { std::move(new_issues) } {
	assert(sprite > 0);
	assert(max_size >= 0);
	assert(merge_max_size >= 0);
}

bool PopType::parse_scripts(GameManager const& game_manager) {
	bool ret = true;
	ret &= country_migration_target.parse_scripts(game_manager);
	ret &= migration_target.parse_scripts(game_manager);
	for (auto [pop_type, weight] : mutable_iterator(promote_to)) {
		ret &= weight.parse_scripts(game_manager);
	}
	for (auto [ideology, weight] : mutable_iterator(ideologies)) {
		ret &= weight.parse_scripts(game_manager);
	}
	for (auto [issue, weight] : mutable_iterator(issues)) {
		ret &= weight.parse_scripts(game_manager);
	}
	return ret;
}

PopManager::PopManager()
  : slave_sprite { 0 }, administrative_sprite { 0 },
	promotion_chance { scope_t::POP, scope_t::POP, scope_t::NO_SCOPE },
	demotion_chance { scope_t::POP, scope_t::POP, scope_t::NO_SCOPE },
	migration_chance { scope_t::POP, scope_t::POP, scope_t::NO_SCOPE },
	colonialmigration_chance { scope_t::POP, scope_t::POP, scope_t::NO_SCOPE },
	emigration_chance { scope_t::POP, scope_t::POP, scope_t::NO_SCOPE },
	assimilation_chance { scope_t::POP, scope_t::POP, scope_t::NO_SCOPE },
	conversion_chance { scope_t::POP, scope_t::POP, scope_t::NO_SCOPE } {}

bool PopManager::add_strata(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid strata identifier - empty!");
		return false;
	}
	return stratas.add_item({ identifier });
}

bool PopManager::add_pop_type(
	std::string_view identifier, colour_t colour, Strata const* strata, PopType::sprite_t sprite,
	Good::good_map_t&& life_needs, Good::good_map_t&& everyday_needs, Good::good_map_t&& luxury_needs,
	PopType::rebel_units_t&& rebel_units, Pop::pop_size_t max_size, Pop::pop_size_t merge_max_size, bool state_capital_only,
	bool demote_migrant, bool is_artisan, bool allowed_to_vote, bool is_slave, bool can_be_recruited,
	bool can_reduce_consciousness, bool administrative_efficiency, bool can_build, bool factory, bool can_work_factory,
	bool unemployment, ConditionalWeight&& country_migration_target, ConditionalWeight&& migration_target,
	ast::NodeCPtr promote_to_node, PopType::ideology_weight_map_t&& ideologies, ast::NodeCPtr issues_node
) {
	if (identifier.empty()) {
		Logger::error("Invalid pop type identifier - empty!");
		return false;
	}
	if (strata == nullptr) {
		Logger::error("Invalid pop type strata for ", identifier, " - null!");
		return false;
	}
	if (sprite <= 0) {
		Logger::error("Invalid pop type sprite index for ", identifier, ": ", sprite);
		return false;
	}
	if (max_size < 0) {
		Logger::error("Invalid pop type max size for ", identifier, ": ", max_size);
		return false;
	}
	if (merge_max_size < 0) {
		Logger::error("Invalid pop type merge max size for ", identifier, ": ", merge_max_size);
		return false;
	}
	const bool ret = pop_types.add_item({
		identifier, colour, *strata, sprite, std::move(life_needs), std::move(everyday_needs),
		std::move(luxury_needs), std::move(rebel_units), max_size, merge_max_size, state_capital_only,
		demote_migrant, is_artisan, allowed_to_vote, is_slave, can_be_recruited, can_reduce_consciousness,
		administrative_efficiency, can_build, factory, can_work_factory, unemployment, std::move(country_migration_target),
		std::move(migration_target), {}, std::move(ideologies), {}
	});
	if (ret) {
		delayed_parse_promote_to_and_issues_nodes.emplace_back(promote_to_node, issues_node);
	}
	if (slave_sprite <= 0 && ret && is_slave) {
		/* Set slave sprite to that of the first is_slave pop type we find. */
		slave_sprite = sprite;
	}
	if (administrative_sprite <= 0 && ret && administrative_efficiency) {
		/* Set administrative sprite to that of the first administrative_efficiency pop type we find. */
		administrative_sprite = sprite;
	}
	return ret;
}

void PopManager::reserve_pop_types(size_t count) {
	stratas.reserve(stratas.size() + count);
	pop_types.reserve(pop_types.size() + count);
}

/* REQUIREMENTS:
 * POP-3, POP-4, POP-5, POP-6, POP-7, POP-8, POP-9, POP-10, POP-11, POP-12, POP-13, POP-14
 */
bool PopManager::load_pop_type_file(
	std::string_view filestem, UnitManager const& unit_manager, GoodManager const& good_manager,
	IdeologyManager const& ideology_manager, ast::NodeCPtr root
) {
	colour_t colour = colour_t::null();
	Strata const* strata = nullptr;
	PopType::sprite_t sprite = 0;
	Good::good_map_t life_needs, everyday_needs, luxury_needs;
	PopType::rebel_units_t rebel_units;
	bool state_capital_only = false, demote_migrant = false, is_artisan = false, allowed_to_vote = true, is_slave = false,
		can_be_recruited = false, can_reduce_consciousness = false, administrative_efficiency = false, can_build = false,
		factory = false, can_work_factory = false, unemployment = false;
	Pop::pop_size_t max_size = 0, merge_max_size = 0;
	ConditionalWeight country_migration_target { scope_t::COUNTRY, scope_t::POP, scope_t::NO_SCOPE };
	ConditionalWeight migration_target { scope_t::PROVINCE, scope_t::POP, scope_t::NO_SCOPE };
	ast::NodeCPtr promote_to_node = nullptr;
	PopType::ideology_weight_map_t ideologies;
	ast::NodeCPtr issues_node = nullptr;

	bool ret = expect_dictionary_keys(
		"sprite", ONE_EXACTLY, expect_uint(assign_variable_callback(sprite)),
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
		"is_artisan", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_artisan)),
		"max_size", ZERO_OR_ONE, expect_uint(assign_variable_callback(max_size)),
		"merge_max_size", ZERO_OR_ONE, expect_uint(assign_variable_callback(merge_max_size)),
		"strata", ONE_EXACTLY, expect_identifier(
			[this, &strata](std::string_view identifier) -> bool {
				strata = get_strata_by_identifier(identifier);
				if (strata != nullptr) {
					return true;
				}
				if (add_strata(identifier)) {
					strata = &get_stratas().back();
					return true;
				}
				return false;
			}
		),
		"state_capital_only", ZERO_OR_ONE, expect_bool(assign_variable_callback(state_capital_only)),
		"research_points", ZERO_OR_ONE, success_callback, // TODO - research points generation
		"research_optimum", ZERO_OR_ONE, success_callback, // TODO - bonus research points generation
		"rebel", ZERO_OR_ONE, unit_manager.expect_unit_decimal_map(move_variable_callback(rebel_units)),
		"equivalent", ZERO_OR_ONE, success_callback, // TODO - worker convertability
		"leadership", ZERO_OR_ONE, success_callback, // TODO - leadership points generation
		"allowed_to_vote", ZERO_OR_ONE, expect_bool(assign_variable_callback(allowed_to_vote)),
		"is_slave", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_slave)),
		"can_be_recruited", ZERO_OR_ONE, expect_bool(assign_variable_callback(can_be_recruited)),
		"can_reduce_consciousness", ZERO_OR_ONE, expect_bool(assign_variable_callback(can_reduce_consciousness)),
		"life_needs_income", ZERO_OR_ONE, success_callback, // TODO - incomes from national budget
		"everyday_needs_income", ZERO_OR_ONE, success_callback,
		"luxury_needs_income", ZERO_OR_ONE, success_callback,
		"luxury_needs", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(luxury_needs)),
		"everyday_needs", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(everyday_needs)),
		"life_needs", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(life_needs)),
		"country_migration_target", ZERO_OR_ONE, country_migration_target.expect_conditional_weight(ConditionalWeight::FACTOR),
		"migration_target", ZERO_OR_ONE, migration_target.expect_conditional_weight(ConditionalWeight::FACTOR),
		"promote_to", ZERO_OR_ONE, assign_variable_callback(promote_to_node),
		"ideologies", ZERO_OR_ONE, ideology_manager.expect_ideology_dictionary_reserve_length(
			ideologies,
			[&filestem, &ideologies](Ideology const& ideology, ast::NodeCPtr node) -> bool {
				ConditionalWeight weight { scope_t::POP, scope_t::POP, scope_t::NO_SCOPE };
				bool ret = weight.expect_conditional_weight(ConditionalWeight::FACTOR)(node);
				ret &= map_callback(ideologies, &ideology)(std::move(weight));
				return ret;
			}
		),
		"issues", ZERO_OR_ONE, assign_variable_callback(issues_node),
		"demote_migrant", ZERO_OR_ONE, expect_bool(assign_variable_callback(demote_migrant)),
		"administrative_efficiency", ZERO_OR_ONE, expect_bool(assign_variable_callback(administrative_efficiency)),
		"tax_eff", ZERO_OR_ONE, success_callback, // TODO - tax collection modifier
		"can_build", ZERO_OR_ONE, expect_bool(assign_variable_callback(can_build)),
		"factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(factory)),
		"workplace_input", ZERO_OR_ONE, success_callback, // TODO - work out what these do
		"workplace_output", ZERO_OR_ONE, success_callback,
		"starter_share", ZERO_OR_ONE, success_callback,
		"can_work_factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(can_work_factory)),
		"unemployment", ZERO_OR_ONE, expect_bool(assign_variable_callback(unemployment))
	)(root);

	ret &= add_pop_type(
		filestem, colour, strata, sprite, std::move(life_needs), std::move(everyday_needs), std::move(luxury_needs),
		std::move(rebel_units), max_size, merge_max_size, state_capital_only, demote_migrant, is_artisan, allowed_to_vote,
		is_slave, can_be_recruited, can_reduce_consciousness, administrative_efficiency, can_build, factory, can_work_factory,
		unemployment, std::move(country_migration_target), std::move(migration_target), promote_to_node, std::move(ideologies),
		issues_node
	);
	return ret;
}

bool PopManager::load_delayed_parse_pop_type_data(IssueManager const& issue_manager) {
	bool ret = true;
	for (size_t index = 0; index < delayed_parse_promote_to_and_issues_nodes.size(); ++index) {
		const auto [promote_to_node, issues_node] = delayed_parse_promote_to_and_issues_nodes[index];
		PopType* pop_type = pop_types.get_item_by_index(index);
		if (promote_to_node != nullptr && !expect_pop_type_dictionary_reserve_length(
			pop_type->promote_to,
			[pop_type](PopType const& type, ast::NodeCPtr node) -> bool {
				if (pop_type == &type) {
					Logger::error("Pop type ", type, " cannot have promotion weight to itself!");
					return false;
				}
				ConditionalWeight weight { scope_t::POP, scope_t::POP, scope_t::NO_SCOPE };
				bool ret = weight.expect_conditional_weight(ConditionalWeight::FACTOR)(node);
				ret &= map_callback(pop_type->promote_to, &type)(std::move(weight));
				return ret;
			}
		)(promote_to_node)) {
			Logger::error("Errors parsing pop type ", pop_type, " promotion weights!");
			ret = false;
		}
		if (issues_node != nullptr && !expect_dictionary_reserve_length(
			pop_type->issues,
			[pop_type, &issue_manager](std::string_view key, ast::NodeCPtr node) -> bool {
				Issue const* issue = issue_manager.get_issue_by_identifier(key);
				if (issue == nullptr) {
					issue = issue_manager.get_reform_by_identifier(key);
				}
				if (issue == nullptr) {
					Logger::error("Invalid issue in pop type ", pop_type, " issue weights: ", key);
					return false;
				}
				ConditionalWeight weight { scope_t::POP, scope_t::POP, scope_t::NO_SCOPE };
				bool ret = weight.expect_conditional_weight(ConditionalWeight::FACTOR)(node);
				ret &= map_callback(pop_type->issues, issue)(std::move(weight));
				return ret;
			}
		)(issues_node)) {
			Logger::error("Errors parsing pop type ", pop_type, " issue weights!");
			ret = false;
		}
	}
	delayed_parse_promote_to_and_issues_nodes.clear();
	return ret;
}

bool PopManager::load_pop_type_chances_file(ast::NodeCPtr root) {
	return expect_dictionary_keys(
		"promotion_chance", ONE_EXACTLY, promotion_chance.expect_conditional_weight(ConditionalWeight::FACTOR),
		"demotion_chance", ONE_EXACTLY, demotion_chance.expect_conditional_weight(ConditionalWeight::FACTOR),
		"migration_chance", ONE_EXACTLY, migration_chance.expect_conditional_weight(ConditionalWeight::FACTOR),
		"colonialmigration_chance", ONE_EXACTLY, colonialmigration_chance.expect_conditional_weight(ConditionalWeight::FACTOR),
		"emigration_chance", ONE_EXACTLY, emigration_chance.expect_conditional_weight(ConditionalWeight::FACTOR),
		"assimilation_chance", ONE_EXACTLY, assimilation_chance.expect_conditional_weight(ConditionalWeight::FACTOR),
		"conversion_chance", ONE_EXACTLY, conversion_chance.expect_conditional_weight(ConditionalWeight::FACTOR)
	)(root);
}

bool PopManager::load_pop_into_vector(
	RebelManager const& rebel_manager, std::vector<Pop>& vec, PopType const& type, ast::NodeCPtr pop_node,
	bool *non_integer_size
) const {
	Culture const* culture = nullptr;
	Religion const* religion = nullptr;
	fixed_point_t size = 0; /* Some genius filled later start dates with non-integer sized pops */
	fixed_point_t militancy = 0, consciousness = 0;
	RebelType const* rebel_type = nullptr;

	bool ret = expect_dictionary_keys(
		"culture", ONE_EXACTLY, culture_manager.expect_culture_identifier(assign_variable_callback_pointer(culture)),
		"religion", ONE_EXACTLY, religion_manager.expect_religion_identifier(assign_variable_callback_pointer(religion)),
		"size", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(size)),
		"militancy", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(militancy)),
		"consciousness", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(consciousness)),
		"rebel_type", ZERO_OR_ONE, rebel_manager.expect_rebel_type_identifier(assign_variable_callback_pointer(rebel_type))
	)(pop_node);

	if (non_integer_size != nullptr && !size.is_integer()) {
		*non_integer_size = true;
	}

	if (culture != nullptr && religion != nullptr && size >= 1) {
		vec.emplace_back(Pop { type, *culture, *religion, size.to_int64_t(), militancy, consciousness, rebel_type });
	} else {
		Logger::warning(
			"Some pop arguments are invalid: culture = ", culture, ", religion = ", religion, ", size = ", size
		);
	}
	return ret;
}

bool PopManager::generate_modifiers(ModifierManager& modifier_manager) const {
	bool ret = true;
	for (Strata const& strata : get_stratas()) {
		const auto strata_modifier = [&modifier_manager, &ret, &strata](std::string_view suffix, bool positive_good) -> void {
			ret &= modifier_manager.add_modifier_effect(
				StringUtils::append_string_views(strata.get_identifier(), suffix), positive_good
			);
		};

		strata_modifier("_income_modifier", true);
		strata_modifier("_savings_modifier", true);
		strata_modifier("_vote", true);

		strata_modifier("_life_needs", false);
		strata_modifier("_everyday_needs", false);
		strata_modifier("_luxury_needs", false);
	}
	return ret;
}

bool PopManager::parse_scripts(GameManager const& game_manager) {
	bool ret = true;
	for (PopType& pop_type : pop_types.get_items()) {
		ret &= pop_type.parse_scripts(game_manager);
	}
	ret &= promotion_chance.parse_scripts(game_manager);
	ret &= demotion_chance.parse_scripts(game_manager);
	ret &= migration_chance.parse_scripts(game_manager);
	ret &= colonialmigration_chance.parse_scripts(game_manager);
	ret &= emigration_chance.parse_scripts(game_manager);
	ret &= assimilation_chance.parse_scripts(game_manager);
	ret &= conversion_chance.parse_scripts(game_manager);
	return ret;
}
