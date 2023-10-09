#include "Pop.hpp"

#include <cassert>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Pop::Pop(PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size)
	: type { new_type },
	  culture { new_culture },
	  religion { new_religion },
	  size { new_size } {
	assert(size > 0);
}

PopType const& Pop::get_type() const {
	return type;
}

Culture const& Pop::get_culture() const {
	return culture;
}

Religion const& Pop::get_religion() const {
	return religion;
}

Pop::pop_size_t Pop::get_size() const {
	return size;
}

Pop::pop_size_t Pop::get_num_promoted() const {
	return num_promoted;
}

Pop::pop_size_t Pop::get_num_demoted() const {
	return num_demoted;
}

Pop::pop_size_t Pop::get_num_migrated() const {
	return num_migrated;
}

Pop::pop_size_t Pop::get_pop_daily_change() const {
	return Pop::get_num_promoted() - (Pop::get_num_demoted() + Pop::get_num_migrated());
}

PopType::PopType(std::string_view new_identifier, colour_t new_colour,
	strata_t new_strata, sprite_t new_sprite,
	Good::good_map_t&& new_life_needs, Good::good_map_t&& new_everyday_needs,
	Good::good_map_t&& new_luxury_needs, rebel_units_t&& new_rebel_units,
	Pop::pop_size_t new_max_size, Pop::pop_size_t new_merge_max_size,
	bool new_state_capital_only, bool new_demote_migrant, bool new_is_artisan, bool new_is_slave)
	: HasIdentifierAndColour { new_identifier, new_colour, true, false },
	  strata { new_strata },
	  sprite { new_sprite },
	  life_needs { std::move(new_life_needs) },
	  everyday_needs { std::move(new_everyday_needs) },
	  luxury_needs { std::move(new_luxury_needs) },
	  rebel_units { std::move(new_rebel_units) },
	  max_size { new_max_size },
	  merge_max_size { new_merge_max_size },
	  state_capital_only { new_state_capital_only },
	  demote_migrant { new_demote_migrant },
	  is_artisan { new_is_artisan },
	  is_slave { new_is_slave } {
	assert(sprite > 0);
	assert(max_size >= 0);
	assert(merge_max_size >= 0);
}

PopType::sprite_t PopType::get_sprite() const {
	return sprite;
}

Good::good_map_t const& PopType::get_life_needs() const {
	return life_needs;
}

Good::good_map_t const& PopType::get_everyday_needs() const {
	return everyday_needs;
}

Good::good_map_t const& PopType::get_luxury_needs() const {
	return luxury_needs;
}

PopType::rebel_units_t const& PopType::get_rebel_units() const {
	return rebel_units;
}

PopType::strata_t PopType::get_strata() const {
	return strata;
}

Pop::pop_size_t PopType::get_max_size() const {
	return max_size;
}

Pop::pop_size_t PopType::get_merge_max_size() const {
	return merge_max_size;
}

bool PopType::get_state_capital_only() const {
	return state_capital_only;
}

bool PopType::get_demote_migrant() const {
	return demote_migrant;
}

bool PopType::get_is_artisan() const {
	return is_artisan;
}

bool PopType::get_is_slave() const {
	return is_slave;
}

PopManager::PopManager() : pop_types { "pop types" } {}

CultureManager& PopManager::get_culture_manager() {
	return culture_manager;
}

CultureManager const& PopManager::get_culture_manager() const {
	return culture_manager;
}

ReligionManager& PopManager::get_religion_manager() {
	return religion_manager;
}

ReligionManager const& PopManager::get_religion_manager() const {
	return religion_manager;
}

bool PopManager::add_pop_type(std::string_view identifier, colour_t colour, PopType::strata_t strata,
	PopType::sprite_t sprite, Good::good_map_t&& life_needs, Good::good_map_t&& everyday_needs,
	Good::good_map_t&& luxury_needs, PopType::rebel_units_t&& rebel_units, Pop::pop_size_t max_size,
	Pop::pop_size_t merge_max_size, bool state_capital_only, bool demote_migrant, bool is_artisan, bool is_slave) {
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
	return pop_types.add_item({ identifier, colour, strata, sprite, std::move(life_needs), std::move(everyday_needs),
		std::move(luxury_needs), std::move(rebel_units), max_size, merge_max_size, state_capital_only, demote_migrant,
		is_artisan, is_slave });
}

/* REQUIREMENTS:
 * POP-3, POP-4, POP-5, POP-6, POP-7, POP-8, POP-9, POP-10, POP-11, POP-12, POP-13, POP-14
 */
bool PopManager::load_pop_type_file(std::string_view filestem, UnitManager const& unit_manager, GoodManager const& good_manager, ast::NodeCPtr root) {
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
	bool state_capital_only = false, is_artisan = false, is_slave = false, demote_migrant = false;
	Pop::pop_size_t max_size = 0, merge_max_size = 0;
	bool ret = expect_dictionary_keys(
		"sprite", ONE_EXACTLY, expect_uint(assign_variable_callback(sprite)),
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
		"is_artisan", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_artisan)),
		"max_size", ZERO_OR_ONE, expect_uint(assign_variable_callback(max_size)),
		"merge_max_size", ZERO_OR_ONE, expect_uint(assign_variable_callback(merge_max_size)),
		"strata", ONE_EXACTLY, expect_identifier(expect_mapped_string(strata_map, assign_variable_callback(strata))),
		"state_capital_only", ZERO_OR_ONE, expect_bool(assign_variable_callback(state_capital_only)),
		"research_points", ZERO_OR_ONE, success_callback,
		"research_optimum", ZERO_OR_ONE, success_callback,
		"rebel", ZERO_OR_ONE, unit_manager.expect_unit_decimal_map(move_variable_callback(rebel_units)),
		"equivalent", ZERO_OR_ONE, success_callback,
		"leadership", ZERO_OR_ONE, success_callback,
		"allowed_to_vote", ZERO_OR_ONE, success_callback,
		"is_slave", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_slave)),
		"can_be_recruited", ZERO_OR_ONE, success_callback,
		"can_reduce_consciousness", ZERO_OR_ONE, success_callback,
		"life_needs_income", ZERO_OR_ONE, success_callback,
		"everyday_needs_income", ZERO_OR_ONE, success_callback,
		"luxury_needs_income", ZERO_OR_ONE, success_callback,
		"luxury_needs", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(luxury_needs)),
		"everyday_needs", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(everyday_needs)),
		"life_needs", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(life_needs)),
		"country_migration_target", ZERO_OR_ONE, success_callback,
		"migration_target", ZERO_OR_ONE, success_callback,
		"promote_to", ZERO_OR_ONE, success_callback,
		"ideologies", ZERO_OR_ONE, success_callback,
		"issues", ZERO_OR_ONE, success_callback,
		"demote_migrant", ZERO_OR_ONE, expect_bool(assign_variable_callback(demote_migrant)),
		"administrative_efficiency", ZERO_OR_ONE, success_callback,
		"tax_eff", ZERO_OR_ONE, success_callback,
		"can_build", ZERO_OR_ONE, success_callback,
		"factory", ZERO_OR_ONE, success_callback,
		"workplace_input", ZERO_OR_ONE, success_callback,
		"workplace_output", ZERO_OR_ONE, success_callback,
		"starter_share", ZERO_OR_ONE, success_callback,
		"can_work_factory", ZERO_OR_ONE, success_callback,
		"unemployment", ZERO_OR_ONE, success_callback
	)(root);

	ret &= add_pop_type(filestem, colour, strata, sprite, std::move(life_needs), std::move(everyday_needs),
		std::move(luxury_needs), std::move(rebel_units), max_size, merge_max_size, state_capital_only, demote_migrant,
		is_artisan, is_slave);
	return ret;
}

bool PopManager::load_pop_into_province(Province& province, std::string_view pop_type_identifier, ast::NodeCPtr pop_node) const {
	PopType const* type = get_pop_type_by_identifier(pop_type_identifier);
	Culture const* culture = nullptr;
	Religion const* religion = nullptr;
	Pop::pop_size_t size = 0;
	bool ret = expect_dictionary_keys(
		"culture", ONE_EXACTLY, expect_identifier(culture_manager.expect_culture_identifier(assign_variable_callback_pointer(culture))),
		"religion", ONE_EXACTLY, expect_identifier(religion_manager.expect_religion_identifier(assign_variable_callback_pointer(religion))),
		"size", ONE_EXACTLY, expect_uint(assign_variable_callback(size)),
		"militancy", ZERO_OR_ONE, success_callback,
		"rebel_type", ZERO_OR_ONE, success_callback
	)(pop_node);

	if (type != nullptr && culture != nullptr && religion != nullptr && size > 0) {
		ret &= province.add_pop({ *type, *culture, *religion, size });
	} else {
		Logger::warning("Some pop arguments are invalid: province = ", province, ", type = ", type, ", culture = ", culture, ", religion = ", religion, ", size = ", size);
	}
	return ret;
}
