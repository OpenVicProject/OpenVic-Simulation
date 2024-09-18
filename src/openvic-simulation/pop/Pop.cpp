#include "Pop.hpp"

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/misc/Define.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"
#include "openvic-simulation/politics/Rebel.hpp"
#include "openvic-simulation/utility/TslHelper.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

using enum PopType::income_type_t;

PopBase::PopBase(
	PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size,
	fixed_point_t new_militancy, fixed_point_t new_consciousness, RebelType const* new_rebel_type
) : type { new_type }, culture { new_culture }, religion { new_religion }, size { new_size }, militancy { new_militancy },
	consciousness { new_consciousness }, rebel_type { new_rebel_type } {}

Pop::Pop(PopBase const& pop_base, decltype(ideologies)::keys_t const& ideology_keys)
  : PopBase { pop_base },
	location { nullptr },
	total_change { 0 },
	num_grown { 0 },
	num_promoted { 0 },
	num_demoted { 0 },
	num_migrated_internal { 0 },
	num_migrated_external { 0 },
	num_migrated_colonial { 0 },
	ideologies { &ideology_keys },
	issues {},
	votes { nullptr },
	unemployment { 0 },
	cash { 0 },
	income { 0 },
	expenses { 0 },
	savings { 0 },
	life_needs_fulfilled { 0 },
	everyday_needs_fulfilled { 0 },
	luxury_needs_fulfilled { 0 },
	max_supported_regiments { 0 } {}

void Pop::setup_pop_test_values(IssueManager const& issue_manager) {
	/* Returns +/- range% of size. */
	const auto test_size = [this](int32_t range) -> pop_size_t {
		return size * ((rand() % (2 * range + 1)) - range) / 100;
	};

	num_grown = test_size(5);
	num_promoted = test_size(1);
	num_demoted = test_size(1);
	num_migrated_internal = test_size(3);
	num_migrated_external = test_size(1);
	num_migrated_colonial = test_size(2);

	total_change =
		num_grown + num_promoted + num_demoted + num_migrated_internal + num_migrated_external + num_migrated_colonial;

	/* Generates a number between 0 and max (inclusive) and sets map[&key] to it if it's at least min. */
	auto test_weight =
		[]<typename T, typename U>(T& map, U const& key, int32_t min, int32_t max) -> void {
			const int32_t value = rand() % (max + 1);
			if (value >= min) {
				if constexpr (utility::is_specialization_of_v<T, IndexedMap>) {
					map[key] = value;
				} else {
					map.emplace(&key, value);
				}
			}
		};

	/* All entries equally weighted for testing. */
	ideologies.clear();
	for (Ideology const& ideology : *ideologies.get_keys()) {
		test_weight(ideologies, ideology, 1, 5);
	}
	ideologies.normalise();

	issues.clear();
	for (Issue const& issue : issue_manager.get_issues()) {
		test_weight(issues, issue, 3, 6);
	}
	for (Reform const& reform : issue_manager.get_reforms()) {
		if (!reform.get_reform_group().get_type().is_uncivilised()) {
			test_weight(issues, reform, 3, 6);
		}
	}
	normalise_fixed_point_map(issues);

	if (votes.has_keys()) {
		votes.clear();
		for (CountryParty const& party : *votes.get_keys()) {
			test_weight(votes, party, 4, 10);
		}
		votes.normalise();
	}

	/* Returns a fixed point between 0 and max. */
	const auto test_range = [](fixed_point_t max = 1) -> fixed_point_t {
		return (rand() % 256) * max / 256;
	};

	unemployment = test_range();
	cash = test_range(20);
	income = test_range(5);
	expenses = test_range(5);
	savings = test_range(15);
	life_needs_fulfilled = test_range();
	everyday_needs_fulfilled = test_range();
	luxury_needs_fulfilled = test_range();
}

void Pop::set_location(ProvinceInstance const& new_location) {
	if (location != &new_location) {
		location = &new_location;

		// TODO - update location dependent attributes

		votes.set_keys(
			location->get_owner() != nullptr ? &location->get_owner()->get_country_definition()->get_parties() : nullptr
		);
		// TODO - calculate vote distribution
	}
}

void Pop::update_gamestate(
	DefineManager const& define_manager, CountryInstance const* owner, fixed_point_t const& pop_size_per_regiment_multiplier
) {
	if (type.get_can_be_recruited()) {
		if (
			size < define_manager.get_min_pop_size_for_regiment() || owner == nullptr ||
			!RegimentType::allowed_cultures_check_culture_in_country(owner->get_allowed_regiment_cultures(), culture, *owner)
		) {
			max_supported_regiments = 0;
		} else {
			max_supported_regiments = (fixed_point_t::parse(size) / (
				fixed_point_t::parse(define_manager.get_pop_size_per_regiment()) * pop_size_per_regiment_multiplier
			)).to_int64_t() + 1;
		}
	}
}

Strata::Strata(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

PopType::PopType(
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
) : HasIdentifierAndColour { new_identifier, new_colour, false },
	strata { new_strata },
	sprite { new_sprite },
	life_needs { std::move(new_life_needs) },
	everyday_needs { std::move(new_everyday_needs) },
	luxury_needs { std::move(new_luxury_needs) },
	life_needs_income_types { std::move(new_life_needs_income_types) },
	everyday_needs_income_types { std::move(new_everyday_needs_income_types) },
	luxury_needs_income_types { std::move(new_luxury_needs_income_types) },
	rebel_units { std::move(new_rebel_units) },
	max_size { new_max_size },
	merge_max_size { new_merge_max_size },
	state_capital_only { new_state_capital_only },
	demote_migrant { new_demote_migrant },
	is_artisan { new_is_artisan },
	allowed_to_vote { new_allowed_to_vote },
	is_slave { new_is_slave },
	can_be_recruited { new_can_be_recruited },
	can_reduce_consciousness { new_can_reduce_consciousness },
	administrative_efficiency { new_administrative_efficiency },
	can_invest { new_can_invest },
	factory { new_factory },
	can_work_factory { new_can_work_factory },
	unemployment { new_unemployment },
	research_points { new_research_points },
	leadership_points { new_leadership_points },
	research_leadership_optimum { new_research_leadership_optimum },
	state_administration_multiplier { new_state_administration_multiplier },
	equivalent { new_equivalent },
	country_migration_target { std::move(new_country_migration_target) },
	migration_target { std::move(new_migration_target) },
	promote_to { std::move(new_promote_to) },
	ideologies { std::move(new_ideologies) },
	issues { std::move(new_issues) } {}

bool PopType::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	ret &= country_migration_target.parse_scripts(definition_manager);
	ret &= migration_target.parse_scripts(definition_manager);
	for (auto [pop_type, weight] : mutable_iterator(promote_to)) {
		ret &= weight.parse_scripts(definition_manager);
	}
	for (auto [ideology, weight] : mutable_iterator(ideologies)) {
		ret &= weight.parse_scripts(definition_manager);
	}
	for (auto [issue, weight] : mutable_iterator(issues)) {
		ret &= weight.parse_scripts(definition_manager);
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
	std::string_view identifier,
	colour_t colour,
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
		Logger::error("Invalid pop type sprite index for ", identifier, ": ", sprite, " (must be positive)");
		return false;
	}
	if (max_size <= 0) {
		Logger::error("Invalid pop type max size for ", identifier, ": ", max_size, " (must be positive)");
		return false;
	}
	if (merge_max_size <= 0) {
		Logger::error("Invalid pop type merge max size for ", identifier, ": ", merge_max_size, " (must be positive)");
		return false;
	}

	if (research_leadership_optimum < 0) {
		Logger::error(
			"Invalid pop type research/leadership optimum for ", identifier, ": ", research_leadership_optimum,
			" (cannot be negative)"
		);
		return false;
	}
	if ((research_points != 0 || leadership_points != 0) != (research_leadership_optimum > 0)) {
		Logger::error(
			"Invalid pop type research/leadership points and optimum for ", identifier, ": research = ", research_points,
			", leadership = ", leadership_points, ", optimum = ", research_leadership_optimum,
			" (optimum is positive if and only if at least one of research and leadership is non-zero)"
		);
		return false;
	}

	const bool ret = pop_types.add_item({
		identifier,
		colour,
		*strata,
		sprite,
		std::move(life_needs),
		std::move(everyday_needs),
		std::move(luxury_needs),
		life_needs_income_types,
		everyday_needs_income_types,
		luxury_needs_income_types,
		{},
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
		nullptr,
		std::move(country_migration_target),
		std::move(migration_target),
		{},
		std::move(ideologies),
		{}
	});

	if (ret) {
		delayed_parse_nodes.emplace_back(rebel_units, equivalent, promote_to_node, issues_node);
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

void PopManager::reserve_all_pop_types(size_t size) {
	reserve_more_stratas(size);
	if (pop_types_are_locked()) {
		Logger::error("Failed to reserve space for ", size, " pop types in PopManager - already locked!");
	} else {
		reserve_more_pop_types(size);
		reserve_more(delayed_parse_nodes, size);
	}
}

void PopManager::lock_all_pop_types() {
	lock_stratas();
	lock_pop_types();
}

static NodeCallback auto expect_needs_income(PopType::income_type_t& types) {
	static const string_map_t<PopType::income_type_t> income_type_map {
		{ "administration", ADMINISTRATION },
		{ "education", EDUCATION },
		{ "military", MILITARY },
		{ "reforms", REFORMS }
	};
	return expect_dictionary_keys(
		"type", ONE_OR_MORE, expect_identifier(expect_mapped_string(income_type_map,
			[&types](PopType::income_type_t type) -> bool {
				if (!share_income_type(types, type)) {
					types |= type;
					return true;
				}
				Logger::error("Duplicate income type ", type, " in pop type income types!");
				return false;
			}
		)),
		"weight", ZERO_OR_ONE, success_callback /* Has no effect in game */
	);
}

/* REQUIREMENTS:
 * POP-3, POP-4, POP-5, POP-6, POP-7, POP-8, POP-9, POP-10, POP-11, POP-12, POP-13, POP-14
 */
bool PopManager::load_pop_type_file(
	std::string_view filestem, GoodDefinitionManager const& good_definition_manager, IdeologyManager const& ideology_manager,
	ast::NodeCPtr root
) {
	colour_t colour = colour_t::null();
	Strata const* strata = nullptr;
	PopType::sprite_t sprite = 0;
	GoodDefinition::good_definition_map_t life_needs, everyday_needs, luxury_needs;
	PopType::income_type_t life_needs_income_types = NO_INCOME_TYPE, everyday_needs_income_types = NO_INCOME_TYPE,
		luxury_needs_income_types = NO_INCOME_TYPE;
	ast::NodeCPtr rebel_units = nullptr;
	Pop::pop_size_t max_size = Pop::MAX_SIZE, merge_max_size = Pop::MAX_SIZE;
	bool state_capital_only = false, demote_migrant = false, is_artisan = false, allowed_to_vote = true, is_slave = false,
		can_be_recruited = false, can_reduce_consciousness = false, administrative_efficiency = false, can_invest = false,
		factory = false, can_work_factory = false, unemployment = false;
	fixed_point_t research_points = 0, leadership_points = 0, research_leadership_optimum = 0,
		state_administration_multiplier = 0;
	ast::NodeCPtr equivalent = nullptr;
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

bool PopManager::load_delayed_parse_pop_type_data(UnitTypeManager const& unit_type_manager, IssueManager const& issue_manager) {
	bool ret = true;
	for (size_t index = 0; index < delayed_parse_nodes.size(); ++index) {
		const auto [rebel_units, equivalent, promote_to_node, issues_node] = delayed_parse_nodes[index];
		PopType* pop_type = pop_types.get_item_by_index(index);

		if (rebel_units != nullptr && !unit_type_manager.expect_unit_type_decimal_map(
			move_variable_callback(pop_type->rebel_units)
		)(rebel_units)) {
			Logger::error("Errors parsing rebel unit distribution for pop type ", pop_type, "!");
			ret = false;
		}

		if (equivalent != nullptr && !expect_pop_type_identifier(
			assign_variable_callback_pointer(pop_type->equivalent)
		)(equivalent)) {
			Logger::error("Errors parsing equivalent pop type for pop type ", pop_type, "!");
			ret = false;
		}

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
			Logger::error("Errors parsing promotion weights for pop type ", pop_type, "!");
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
			Logger::error("Errors parsing issue weights for pop type ", pop_type, "!");
			ret = false;
		}
	}
	delayed_parse_nodes.clear();
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

bool PopManager::load_pop_bases_into_vector(
	RebelManager const& rebel_manager, std::vector<PopBase>& vec, PopType const& type, ast::NodeCPtr pop_node,
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

	if (culture != nullptr && religion != nullptr && size >= 1 && size <= std::numeric_limits<Pop::pop_size_t>::max()) {
		vec.emplace_back(PopBase { type, *culture, *religion, size.to_int32_t(), militancy, consciousness, rebel_type });
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
		const auto strata_modifier = [&modifier_manager, &ret, &strata](
			std::string_view suffix, bool is_positive_good
		) -> void {
			ret &= modifier_manager.add_modifier_effect(
				StringUtils::append_string_views(strata.get_identifier(), suffix), is_positive_good
			);
		};

		strata_modifier("_income_modifier", true);
		strata_modifier("_vote", true);

		strata_modifier("_life_needs", false);
		strata_modifier("_everyday_needs", false);
		strata_modifier("_luxury_needs", false);
	}

	for (PopType const& pop_type : get_pop_types()) {
		ret &= modifier_manager.add_modifier_effect(pop_type.get_identifier(), true);
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
