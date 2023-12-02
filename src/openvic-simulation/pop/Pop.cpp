#include "Pop.hpp"

#include <cassert>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Pop::Pop(
	PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size
) : type { new_type }, culture { new_culture }, religion { new_religion }, size { new_size } {
	assert(size > 0);
}

Pop::pop_size_t Pop::get_pop_daily_change() const {
	return Pop::get_num_promoted() - (Pop::get_num_demoted() + Pop::get_num_migrated());
}

PopType::PopType(
	std::string_view new_identifier, colour_t new_colour, strata_t new_strata, sprite_t new_sprite,
	Good::good_map_t&& new_life_needs, Good::good_map_t&& new_everyday_needs, Good::good_map_t&& new_luxury_needs,
	rebel_units_t&& new_rebel_units, Pop::pop_size_t new_max_size, Pop::pop_size_t new_merge_max_size,
	bool new_state_capital_only, bool new_demote_migrant, bool new_is_artisan, bool new_allowed_to_vote, bool new_is_slave,
	bool new_can_be_recruited, bool new_can_reduce_consciousness, bool new_administrative_efficiency, bool new_can_build,
	bool new_factory, bool new_can_work_factory, bool new_unemployment
) : HasIdentifierAndColour { new_identifier, new_colour, false, false }, strata { new_strata }, sprite { new_sprite },
	life_needs { std::move(new_life_needs) }, everyday_needs { std::move(new_everyday_needs) },
	luxury_needs { std::move(new_luxury_needs) }, rebel_units { std::move(new_rebel_units) }, max_size { new_max_size },
	merge_max_size { new_merge_max_size }, state_capital_only { new_state_capital_only },
	demote_migrant { new_demote_migrant }, is_artisan { new_is_artisan }, allowed_to_vote { new_allowed_to_vote },
	is_slave { new_is_slave }, can_be_recruited { new_can_be_recruited },
	can_reduce_consciousness { new_can_reduce_consciousness }, administrative_efficiency { new_administrative_efficiency },
	can_build { new_can_build }, factory { new_factory }, can_work_factory { new_can_work_factory },
	unemployment { new_unemployment } {
	assert(sprite > 0);
	assert(max_size >= 0);
	assert(merge_max_size >= 0);
}

PopManager::PopManager() : pop_types { "pop types" }, slave_sprite { 0 }, administrative_sprite { 0 } {}

bool PopManager::add_pop_type(
	std::string_view identifier, colour_t colour, PopType::strata_t strata, PopType::sprite_t sprite,
	Good::good_map_t&& life_needs, Good::good_map_t&& everyday_needs, Good::good_map_t&& luxury_needs,
	PopType::rebel_units_t&& rebel_units, Pop::pop_size_t max_size, Pop::pop_size_t merge_max_size, bool state_capital_only,
	bool demote_migrant, bool is_artisan, bool allowed_to_vote, bool is_slave, bool can_be_recruited,
	bool can_reduce_consciousness, bool administrative_efficiency, bool can_build, bool factory, bool can_work_factory,
	bool unemployment
) {
	if (identifier.empty()) {
		Logger::error("Invalid pop type identifier - empty!");
		return false;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid pop type colour for ", identifier, ": ", colour_to_hex_string(colour));
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
		identifier, colour, strata, sprite, std::move(life_needs), std::move(everyday_needs),
		std::move(luxury_needs), std::move(rebel_units), max_size, merge_max_size, state_capital_only,
		demote_migrant, is_artisan, allowed_to_vote, is_slave, can_be_recruited, can_reduce_consciousness,
		administrative_efficiency, can_build, factory, can_work_factory, unemployment
	});
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

/* REQUIREMENTS:
 * POP-3, POP-4, POP-5, POP-6, POP-7, POP-8, POP-9, POP-10, POP-11, POP-12, POP-13, POP-14
 */
bool PopManager::load_pop_type_file(
	std::string_view filestem, UnitManager const& unit_manager, GoodManager const& good_manager, ast::NodeCPtr root
) {
	static const string_map_t<PopType::strata_t> strata_map = {
		{ "poor", PopType::strata_t::POOR },
		{ "middle", PopType::strata_t::MIDDLE },
		{ "rich", PopType::strata_t::RICH }
	};

	colour_t colour = NULL_COLOUR;
	PopType::strata_t strata = PopType::strata_t::POOR;
	PopType::sprite_t sprite = 0;
	Good::good_map_t life_needs, everyday_needs, luxury_needs;
	PopType::rebel_units_t rebel_units;
	bool state_capital_only = false, demote_migrant = false, is_artisan = false, allowed_to_vote = true, is_slave = false,
		can_be_recruited = false, can_reduce_consciousness = false, administrative_efficiency = false, can_build = false,
		factory = false, can_work_factory = false, unemployment = false;
	Pop::pop_size_t max_size = 0, merge_max_size = 0;
	bool ret = expect_dictionary_keys(
		"sprite", ONE_EXACTLY, expect_uint(assign_variable_callback(sprite)),
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
		"is_artisan", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_artisan)),
		"max_size", ZERO_OR_ONE, expect_uint(assign_variable_callback(max_size)),
		"merge_max_size", ZERO_OR_ONE, expect_uint(assign_variable_callback(merge_max_size)),
		"strata", ONE_EXACTLY, expect_identifier(expect_mapped_string(strata_map, assign_variable_callback(strata))),
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
		"country_migration_target", ZERO_OR_ONE, success_callback, // TODO - pop migration weight scripts
		"migration_target", ZERO_OR_ONE, success_callback,
		"promote_to", ZERO_OR_ONE, success_callback, // TODO - pop promotion weight scripts
		"ideologies", ZERO_OR_ONE, success_callback, // TODO - pop politics weight scripts
		"issues", ZERO_OR_ONE, success_callback,
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
		unemployment
	);
	return ret;
}

bool PopManager::load_pop_into_province(Province& province, std::string_view pop_type_identifier, ast::NodeCPtr pop_node)
	const {
	PopType const* type = get_pop_type_by_identifier(pop_type_identifier);
	Culture const* culture = nullptr;
	Religion const* religion = nullptr;
	Pop::pop_size_t size = 0;
	bool ret = expect_dictionary_keys(
		"culture", ONE_EXACTLY, culture_manager.expect_culture_identifier(assign_variable_callback_pointer(culture)),
		"religion", ONE_EXACTLY, religion_manager.expect_religion_identifier(assign_variable_callback_pointer(religion)),
		"size", ONE_EXACTLY, expect_uint(assign_variable_callback(size)),
		"militancy", ZERO_OR_ONE, success_callback,
		"rebel_type", ZERO_OR_ONE, success_callback
	)(pop_node);

	if (type != nullptr && culture != nullptr && religion != nullptr && size > 0) {
		ret &= province.add_pop({ *type, *culture, *religion, size });
	} else {
		Logger::warning(
			"Some pop arguments are invalid: province = ", province, ", type = ", type, ", culture = ", culture,
			", religion = ", religion, ", size = ", size
		);
	}
	return ret;
}
