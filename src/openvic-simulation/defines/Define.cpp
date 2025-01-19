#include "Define.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

DefineManager::DefineManager() : start_date {}, end_date {} {}

bool DefineManager::load_defines_file(ast::NodeCPtr root) {
	bool ret = expect_dictionary_keys(
		"defines", ONE_EXACTLY, expect_dictionary_keys(
			"start_date", ONE_EXACTLY, expect_date_identifier_or_string(assign_variable_callback(start_date)),
			"end_date", ONE_EXACTLY, expect_date_identifier_or_string(assign_variable_callback(end_date)),

			ai_defines.get_name(), ONE_EXACTLY, ai_defines.expect_defines(),
			country_defines.get_name(), ONE_EXACTLY, country_defines.expect_defines(),
			diplomacy_defines.get_name(), ONE_EXACTLY, diplomacy_defines.expect_defines(),
			economy_defines.get_name(), ONE_EXACTLY, economy_defines.expect_defines(),
			graphics_defines.get_name(), ONE_EXACTLY, graphics_defines.expect_defines(),
			military_defines.get_name(), ONE_EXACTLY, military_defines.expect_defines(),
			pops_defines.get_name(), ONE_EXACTLY, pops_defines.expect_defines()
		)
	)(root);

	// TODO - move to MilitaryDefines?

	if (military_defines.get_leader_recruit_cost() <= fixed_point_t::_0()) {
		Logger::error(
			"Invalid leader recruit cost: ", military_defines.get_leader_recruit_cost(),
			" - must be greater than 0, changing to 1"
		);
		ret = false;
		military_defines.leader_recruit_cost = fixed_point_t::_1();
	}

	return ret;
}
