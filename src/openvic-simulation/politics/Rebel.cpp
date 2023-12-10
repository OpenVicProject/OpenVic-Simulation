#include "Rebel.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

RebelType::RebelType(
	std::string_view new_identifier, RebelType::icon_t icon, RebelType::area_t area, bool break_alliance_on_win,
	RebelType::government_map_t&& desired_governments, RebelType::defection_t defection,
	RebelType::independence_t independence, uint16_t defect_delay, Ideology const* ideology, bool allow_all_cultures,
	bool allow_all_culture_groups, bool allow_all_religions, bool allow_all_ideologies, bool resilient, bool reinforcing,
	bool general, bool smart, bool unit_transfer, fixed_point_t occupation_mult
) : HasIdentifier { new_identifier }, icon { icon }, area { area }, break_alliance_on_win { break_alliance_on_win },
	desired_governments { std::move(desired_governments) }, defection { defection }, independence { independence },
	defect_delay { defect_delay }, ideology { ideology }, allow_all_cultures { allow_all_cultures },
	allow_all_culture_groups { allow_all_culture_groups }, allow_all_religions { allow_all_religions },
	allow_all_ideologies { allow_all_ideologies }, resilient { resilient }, reinforcing { reinforcing }, general { general },
	smart { smart }, unit_transfer { unit_transfer }, occupation_mult { occupation_mult } {}

bool RebelManager::add_rebel_type(
	std::string_view new_identifier, RebelType::icon_t icon, RebelType::area_t area, bool break_alliance_on_win,
	RebelType::government_map_t&& desired_governments, RebelType::defection_t defection,
	RebelType::independence_t independence, uint16_t defect_delay, Ideology const* ideology, bool allow_all_cultures,
	bool allow_all_culture_groups, bool allow_all_religions, bool allow_all_ideologies, bool resilient, bool reinforcing,
	bool general, bool smart, bool unit_transfer, fixed_point_t occupation_mult
) {
	if (new_identifier.empty()) {
		Logger::error("Invalid rebel type identifier - empty!");
		return false;
	}

	return rebel_types.add_item({
		new_identifier, icon, area, break_alliance_on_win, std::move(desired_governments), defection, independence,
		defect_delay, ideology, allow_all_cultures, allow_all_culture_groups, allow_all_religions, allow_all_ideologies,
		resilient, reinforcing, general, smart, unit_transfer, occupation_mult
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
			RebelType::defection_t defection = RebelType::defection_t::ANY;
			RebelType::independence_t independence = RebelType::independence_t::ANY;
			uint16_t defect_delay = 0;
			Ideology const* ideology = nullptr;
			bool break_alliance_on_win = false, allow_all_cultures = true, allow_all_culture_groups = true,
				allow_all_religions = true, allow_all_ideologies = true, resilient = true, reinforcing = true, general = true,
				smart = true, unit_transfer = false;
			fixed_point_t occupation_mult = 0;

			bool ret = expect_dictionary_keys(
				"icon", ONE_EXACTLY, expect_uint(assign_variable_callback(icon)),
				"area", ONE_EXACTLY, expect_identifier(expect_mapped_string(area_map, assign_variable_callback(area))),
				"break_alliance_on_win", ZERO_OR_ONE, expect_bool(assign_variable_callback(break_alliance_on_win)),
				"government", ONE_EXACTLY, government_type_manager.expect_government_type_dictionary(
					[this, &government_type_manager, &desired_governments](GovernmentType const& from,
						ast::NodeCPtr value) -> bool {
						if (desired_governments.contains(&from)) {
							Logger::error("Duplicate \"from\" government type in rebel type: ", from.get_identifier());
							return false;
						}
						return government_type_manager.expect_government_type_identifier(
							[&desired_governments, &from](GovernmentType const& to) -> bool {
								desired_governments.emplace(&from, &to);
								return true;
							}
						)(value);
					}
				),
				"defection", ONE_EXACTLY,
					expect_identifier(expect_mapped_string(defection_map, assign_variable_callback(defection))),
				"independence", ONE_EXACTLY,
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
				"will_rise", ONE_EXACTLY, success_callback, //TODO
				"spawn_chance", ONE_EXACTLY, success_callback, //TODO
				"movement_evaluation", ONE_EXACTLY, success_callback, //TODO
				"siege_won_trigger", ZERO_OR_ONE, success_callback, //TODO
				"siege_won_effect", ZERO_OR_ONE, success_callback, //TODO
				"demands_enforced_trigger", ZERO_OR_ONE, success_callback, //TODO
				"demands_enforced_effect", ZERO_OR_ONE, success_callback //TODO
			)(node);

			ret &= add_rebel_type(
				identifier, icon, area, break_alliance_on_win, std::move(desired_governments), defection, independence,
				defect_delay, ideology, allow_all_cultures, allow_all_culture_groups, allow_all_religions,
				allow_all_ideologies, resilient, reinforcing, general, smart, unit_transfer, occupation_mult
			);

			return ret;
		}
	)(root);

	lock_rebel_types();

	return ret;
}

bool RebelManager::generate_modifiers(ModifierManager& modifier_manager) {
	bool ret = true;

	modifier_manager.register_complex_modifier("rebel_org_gain");

	ret &= modifier_manager.add_modifier_effect("rebel_org_gain_all", false);

	for (RebelType const& rebel_type : get_rebel_types()) {
		std::string modifier_name = StringUtils::append_string_views("rebel_org_gain_", rebel_type.get_identifier());
		ret &= modifier_manager.add_modifier_effect(modifier_name, false);
	}
	return ret;
}