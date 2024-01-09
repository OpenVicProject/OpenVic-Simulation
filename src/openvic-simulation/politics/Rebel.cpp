#include "Rebel.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

RebelType::RebelType(
	std::string_view new_identifier, RebelType::icon_t icon, RebelType::area_t area, bool break_alliance_on_win,
	RebelType::government_map_t&& desired_governments, RebelType::defection_t defection,
	RebelType::independence_t independence, uint16_t defect_delay, Ideology const* ideology, bool allow_all_cultures,
	bool allow_all_culture_groups, bool allow_all_religions, bool allow_all_ideologies, bool resilient, bool reinforcing,
	bool general, bool smart, bool unit_transfer, fixed_point_t occupation_mult, ConditionalWeight&& new_will_rise,
	ConditionalWeight&& new_spawn_chance, ConditionalWeight&& new_movement_evaluation, ConditionScript&& new_siege_won_trigger,
	EffectScript&& new_siege_won_effect, ConditionScript&& new_demands_enforced_trigger,
	EffectScript&& new_demands_enforced_effect
) : HasIdentifier { new_identifier }, icon { icon }, area { area }, break_alliance_on_win { break_alliance_on_win },
	desired_governments { std::move(desired_governments) }, defection { defection }, independence { independence },
	defect_delay { defect_delay }, ideology { ideology }, allow_all_cultures { allow_all_cultures },
	allow_all_culture_groups { allow_all_culture_groups }, allow_all_religions { allow_all_religions },
	allow_all_ideologies { allow_all_ideologies }, resilient { resilient }, reinforcing { reinforcing }, general { general },
	smart { smart }, unit_transfer { unit_transfer }, occupation_mult { occupation_mult },
	will_rise { std::move(new_will_rise) }, spawn_chance { std::move(new_spawn_chance) },
	movement_evaluation { std::move(new_movement_evaluation) }, siege_won_trigger { std::move(new_siege_won_trigger) },
	siege_won_effect { std::move(new_siege_won_effect) }, demands_enforced_trigger { std::move(new_demands_enforced_trigger) },
	demands_enforced_effect { std::move(new_demands_enforced_effect) } {}

bool RebelType::parse_scripts(GameManager& game_manager) {
	bool ret = true;
	ret &= will_rise.parse_scripts(game_manager);
	ret &= spawn_chance.parse_scripts(game_manager);
	ret &= movement_evaluation.parse_scripts(game_manager);
	ret &= siege_won_trigger.parse_script(true, game_manager);
	ret &= siege_won_effect.parse_script(true, game_manager);
	ret &= demands_enforced_trigger.parse_script(true, game_manager);
	ret &= demands_enforced_effect.parse_script(true, game_manager);
	return ret;
}

bool RebelManager::add_rebel_type(
	std::string_view new_identifier, RebelType::icon_t icon, RebelType::area_t area, bool break_alliance_on_win,
	RebelType::government_map_t&& desired_governments, RebelType::defection_t defection,
	RebelType::independence_t independence, uint16_t defect_delay, Ideology const* ideology, bool allow_all_cultures,
	bool allow_all_culture_groups, bool allow_all_religions, bool allow_all_ideologies, bool resilient, bool reinforcing,
	bool general, bool smart, bool unit_transfer, fixed_point_t occupation_mult, ConditionalWeight&& will_rise,
	ConditionalWeight&& spawn_chance, ConditionalWeight&& movement_evaluation, ConditionScript&& siege_won_trigger,
	EffectScript&& siege_won_effect, ConditionScript&& demands_enforced_trigger, EffectScript&& demands_enforced_effect
) {
	if (new_identifier.empty()) {
		Logger::error("Invalid rebel type identifier - empty!");
		return false;
	}

	return rebel_types.add_item({
		new_identifier, icon, area, break_alliance_on_win, std::move(desired_governments), defection, independence,
		defect_delay, ideology, allow_all_cultures, allow_all_culture_groups, allow_all_religions, allow_all_ideologies,
		resilient, reinforcing, general, smart, unit_transfer, occupation_mult, std::move(will_rise), std::move(spawn_chance),
		std::move(movement_evaluation), std::move(siege_won_trigger), std::move(siege_won_effect),
		std::move(demands_enforced_trigger), std::move(demands_enforced_effect)
	});
}

bool RebelManager::load_rebels_file(
	IdeologyManager const& ideology_manager, GovernmentTypeManager const& government_type_manager, ast::NodeCPtr root
) {
	static const string_map_t<RebelType::area_t> area_map = {
		{ "nation", RebelType::area_t::NATION },
		{ "nation_religion", RebelType::area_t::NATION_RELIGION },
		{ "nation_culture", RebelType::area_t::NATION_CULTURE },
		{ "culture", RebelType::area_t::CULTURE },
		{ "culture_group", RebelType::area_t::CULTURE_GROUP },
		{ "religion", RebelType::area_t::RELIGION },
		{ "all", RebelType::area_t::ALL }
	};

	static const string_map_t<RebelType::defection_t> defection_map = {
		{ "none", RebelType::defection_t::NONE },
		{ "culture", RebelType::defection_t::CULTURE },
		{ "culture_group", RebelType::defection_t::CULTURE_GROUP },
		{ "religion", RebelType::defection_t::RELIGION },
		{ "ideology", RebelType::defection_t::IDEOLOGY },
		{ "pan_nationalist", RebelType::defection_t::PAN_NATIONALIST },
		{ "any", RebelType::defection_t::ANY }
	};

	static const string_map_t<RebelType::independence_t> independence_map = {
		{ "none", RebelType::independence_t::NONE },
		{ "culture", RebelType::independence_t::CULTURE },
		{ "culture_group", RebelType::independence_t::CULTURE_GROUP },
		{ "religion", RebelType::independence_t::RELIGION },
		{ "colonial", RebelType::independence_t::COLONIAL },
		{ "pan_nationalist", RebelType::independence_t::PAN_NATIONALIST },
		{ "any", RebelType::independence_t::ANY }
	};

	bool ret = expect_dictionary(
		[this, &ideology_manager, &government_type_manager](std::string_view identifier, ast::NodeCPtr node) -> bool {
			RebelType::icon_t icon = 0;
			RebelType::area_t area = RebelType::area_t::ALL;
			RebelType::government_map_t desired_governments;
			RebelType::defection_t defection = RebelType::defection_t::NONE;
			RebelType::independence_t independence = RebelType::independence_t::NONE;
			uint16_t defect_delay = 0;
			Ideology const* ideology = nullptr;
			bool break_alliance_on_win = false, allow_all_cultures = true, allow_all_culture_groups = true,
				allow_all_religions = true, allow_all_ideologies = true, resilient = true, reinforcing = true, general = true,
				smart = true, unit_transfer = false;
			fixed_point_t occupation_mult = 0;
			ConditionalWeight will_rise { scope_t::POP, scope_t::COUNTRY, scope_t::NO_SCOPE };
			ConditionalWeight spawn_chance { scope_t::POP, scope_t::POP, scope_t::NO_SCOPE };
			ConditionalWeight movement_evaluation { scope_t::PROVINCE, scope_t::PROVINCE, scope_t::NO_SCOPE };
			ConditionScript siege_won_trigger { scope_t::PROVINCE, scope_t::PROVINCE, scope_t::NO_SCOPE };
			ConditionScript demands_enforced_trigger { scope_t::COUNTRY, scope_t::COUNTRY, scope_t::NO_SCOPE };
			EffectScript siege_won_effect, demands_enforced_effect;

			bool ret = expect_dictionary_keys(
				"icon", ONE_EXACTLY, expect_uint(assign_variable_callback(icon)),
				"area", ONE_EXACTLY, expect_identifier(expect_mapped_string(area_map, assign_variable_callback(area))),
				"break_alliance_on_win", ZERO_OR_ONE, expect_bool(assign_variable_callback(break_alliance_on_win)),
				"government", ONE_EXACTLY, government_type_manager.expect_government_type_dictionary_reserve_length(
					desired_governments,
					[this, &government_type_manager, &desired_governments](
						GovernmentType const& from, ast::NodeCPtr value
					) -> bool {
						return government_type_manager.expect_government_type_identifier(
							[&desired_governments, &from](GovernmentType const& to) -> bool {
								return map_callback(desired_governments, &from)(&to);
							}
						)(value);
					}
				),
				"defection", ZERO_OR_ONE,
					expect_identifier(expect_mapped_string(defection_map, assign_variable_callback(defection))),
				"independence", ZERO_OR_ONE,
					expect_identifier(expect_mapped_string(independence_map, assign_variable_callback(independence))),
				"defect_delay", ONE_EXACTLY, expect_uint(assign_variable_callback(defect_delay)),
				"ideology", ZERO_OR_ONE,
					ideology_manager.expect_ideology_identifier(assign_variable_callback_pointer(ideology)),
				"allow_all_cultures", ONE_EXACTLY, expect_bool(assign_variable_callback(allow_all_cultures)),
				"allow_all_culture_groups", ZERO_OR_ONE, expect_bool(assign_variable_callback(allow_all_culture_groups)),
				"allow_all_religions", ONE_EXACTLY, expect_bool(assign_variable_callback(allow_all_religions)),
				"allow_all_ideologies", ONE_EXACTLY, expect_bool(assign_variable_callback(allow_all_ideologies)),
				"resilient", ONE_EXACTLY, expect_bool(assign_variable_callback(resilient)),
				"reinforcing", ONE_EXACTLY, expect_bool(assign_variable_callback(reinforcing)),
				"general", ONE_EXACTLY, expect_bool(assign_variable_callback(general)),
				"smart", ONE_EXACTLY, expect_bool(assign_variable_callback(smart)),
				"unit_transfer", ONE_EXACTLY, expect_bool(assign_variable_callback(unit_transfer)),
				"occupation_mult", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(occupation_mult)),
				"will_rise", ONE_EXACTLY, will_rise.expect_conditional_weight(ConditionalWeight::FACTOR),
				"spawn_chance", ONE_EXACTLY, spawn_chance.expect_conditional_weight(ConditionalWeight::FACTOR),
				"movement_evaluation", ONE_EXACTLY, movement_evaluation.expect_conditional_weight(ConditionalWeight::FACTOR),
				"siege_won_trigger", ZERO_OR_ONE, siege_won_trigger.expect_script(),
				"siege_won_effect", ZERO_OR_ONE, siege_won_effect.expect_script(),
				"demands_enforced_trigger", ZERO_OR_ONE, demands_enforced_trigger.expect_script(),
				"demands_enforced_effect", ZERO_OR_ONE, demands_enforced_effect.expect_script()
			)(node);

			ret &= add_rebel_type(
				identifier, icon, area, break_alliance_on_win, std::move(desired_governments), defection, independence,
				defect_delay, ideology, allow_all_cultures, allow_all_culture_groups, allow_all_religions,
				allow_all_ideologies, resilient, reinforcing, general, smart, unit_transfer, occupation_mult,
				std::move(will_rise), std::move(spawn_chance), std::move(movement_evaluation), std::move(siege_won_trigger),
				std::move(siege_won_effect), std::move(demands_enforced_trigger), std::move(demands_enforced_effect)
			);

			return ret;
		}
	)(root);

	lock_rebel_types();

	return ret;
}

bool RebelManager::generate_modifiers(ModifierManager& modifier_manager) const {
	bool ret = true;

	ret &= modifier_manager.register_complex_modifier("rebel_org_gain");

	ret &= modifier_manager.add_modifier_effect("rebel_org_gain_all", false);

	for (RebelType const& rebel_type : get_rebel_types()) {
		ret &= modifier_manager.add_modifier_effect(
			StringUtils::append_string_views("rebel_org_gain_", rebel_type.get_identifier()), false
		);
	}
	return ret;
}

bool RebelManager::parse_scripts(GameManager& game_manager) {
	bool ret = true;
	for (RebelType& rebel_type : rebel_types.get_items()) {
		ret &= rebel_type.parse_scripts(game_manager);
	}
	return ret;
}
