#pragma once

#include <span>
#include <string_view>

#include "openvic-simulation/dataloader/NodeCallbacks.hpp"
#include "openvic-simulation/scripts/Condition.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct ConditionManager;
	struct ConditionScript;
	struct DefinitionManager;

	struct ConditionManager {
	private:
		CaseInsensitiveIdentifierRegistry<Condition> IDENTIFIER_REGISTRY(condition);
		Condition const* root_condition = nullptr;

		bool add_condition(
			std::string_view identifier, value_type_t value_type, scope_type_t scope,
			scope_type_t scope_change = scope_type_t::NO_SCOPE,
			identifier_type_t key_identifier_type = identifier_type_t::NO_IDENTIFIER,
			identifier_type_t value_identifier_type = identifier_type_t::NO_IDENTIFIER
		);

		NodeTools::callback_t<std::string_view> expect_parse_identifier(
			DefinitionManager const& definition_manager, identifier_type_t identifier_type,
			NodeTools::callback_t<HasIdentifier const*> callback
		) const;

		NodeTools::node_callback_t expect_condition_node(
			DefinitionManager const& definition_manager, Condition const& condition, scope_type_t current_scope,
			scope_type_t this_scope, scope_type_t from_scope, NodeTools::callback_t<ConditionNode&&> callback
		) const;

		NodeTools::node_callback_t expect_condition_node_list(
			DefinitionManager const& definition_manager, scope_type_t current_scope, scope_type_t this_scope,
			scope_type_t from_scope, NodeTools::callback_t<ConditionNode&&> callback, bool top_scope = false
		) const;

	public:
		bool setup_conditions(DefinitionManager const& definition_manager);

		bool expect_condition_script(
			DefinitionManager const& definition_manager, scope_type_t initial_scope, scope_type_t this_scope,
			scope_type_t from_scope, NodeTools::callback_t<ConditionNode&&> callback, std::span<ovdl::v2script::ast::Node const* const> nodes
		) const;
	};
}
