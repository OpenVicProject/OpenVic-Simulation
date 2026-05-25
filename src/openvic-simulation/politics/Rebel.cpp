#include "Rebel.hpp"

#include <string_view>

#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"

using namespace OpenVic;

RebelType::RebelType(
	index_t new_index, std::string_view new_identifier,
	RebelType::icon_t icon, RebelType::area_t area, bool break_alliance_on_win,
	RebelType::government_map_t&& desired_governments, RebelType::defection_t defection,
	RebelType::independence_t independence, uint16_t defect_delay, Ideology const* ideology, bool allow_all_cultures,
	bool allow_all_culture_groups, bool allow_all_religions, bool allow_all_ideologies, bool resilient, bool reinforcing,
	bool general, bool smart, bool unit_transfer, fixed_point_t occupation_mult, ConditionalWeightFactorMul&& new_will_rise,
	ConditionalWeightFactorMul&& new_spawn_chance, ConditionalWeightFactorMul&& new_movement_evaluation,
	ConditionScript&& new_siege_won_trigger, EffectScript&& new_siege_won_effect,
	ConditionScript&& new_demands_enforced_trigger, EffectScript&& new_demands_enforced_effect
) : HasIndex { new_index },
	HasIdentifier { new_identifier },
	icon { icon }, area { area }, will_break_alliance_on_win { break_alliance_on_win },
	desired_governments { std::move(desired_governments) }, defection_type { defection }, independence_type { independence },
	defect_delay { defect_delay }, ideology { ideology }, allows_all_cultures { allow_all_cultures },
	allows_all_culture_groups { allow_all_culture_groups }, allows_all_religions { allow_all_religions },
	allows_all_ideologies { allow_all_ideologies }, is_resilient { resilient }, is_reinforcing { reinforcing }, has_generals { general },
	is_smart { smart }, will_transfer_units { unit_transfer }, occupation_mult { occupation_mult },
	will_rise { std::move(new_will_rise) }, spawn_chance { std::move(new_spawn_chance) },
	movement_evaluation { std::move(new_movement_evaluation) }, siege_won_trigger { std::move(new_siege_won_trigger) },
	siege_won_effect { std::move(new_siege_won_effect) }, demands_enforced_trigger { std::move(new_demands_enforced_trigger) },
	demands_enforced_effect { std::move(new_demands_enforced_effect) } {}

bool RebelType::parse_scripts(DefinitionManager const& definition_manager) {
	spdlog::scope scope { fmt::format("rebel type {}", get_identifier()) };
	bool ret = true;
	ret &= will_rise.parse_scripts(definition_manager);
	ret &= spawn_chance.parse_scripts(definition_manager);
	ret &= movement_evaluation.parse_scripts(definition_manager);
	ret &= siege_won_trigger.parse_script(true, definition_manager);
	ret &= siege_won_effect.parse_script(true, definition_manager);
	ret &= demands_enforced_trigger.parse_script(true, definition_manager);
	ret &= demands_enforced_effect.parse_script(true, definition_manager);
	return ret;
}
