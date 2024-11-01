#pragma once

#include "openvic-simulation/scripts/Condition.hpp"
#include "openvic-simulation/scripts/Script.hpp"

namespace OpenVic {
	struct DefinitionManager;

	struct ConditionScript final : Script<DefinitionManager const&> {

	private:
		ConditionNode PROPERTY_REF(condition_root);
		scope_type_t PROPERTY(initial_scope);
		scope_type_t PROPERTY(this_scope);
		scope_type_t PROPERTY(from_scope);

	protected:
		bool _parse_script(ast::NodeCPtr root, DefinitionManager const& definition_manager) override;

	public:
		ConditionScript(scope_type_t new_initial_scope, scope_type_t new_this_scope, scope_type_t new_from_scope);

		bool execute(
			InstanceManager const& instance_manager,
			ConditionNode::scope_t const& initial_scope,
			ConditionNode::scope_t const& this_scope,
			ConditionNode::scope_t const& from_scope
		) const;
	};
}
