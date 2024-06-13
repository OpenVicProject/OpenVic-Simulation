#include "ConditionScript.hpp"

#include "openvic-simulation/DefinitionManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

ConditionScript::ConditionScript(
	scope_t new_initial_scope, scope_t new_this_scope, scope_t new_from_scope
) : initial_scope { new_initial_scope }, this_scope { new_this_scope }, from_scope { new_from_scope } {}

bool ConditionScript::_parse_script(ast::NodeCPtr root, DefinitionManager const& definition_manager) {
	return definition_manager.get_script_manager().get_condition_manager().expect_condition_script(
		definition_manager,
		initial_scope,
		this_scope,
		from_scope,
		move_variable_callback(condition_root)
	)(root);
}
