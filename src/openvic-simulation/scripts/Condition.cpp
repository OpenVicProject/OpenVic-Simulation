#include "Condition.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

using no_argument_t = ConditionNode::no_argument_t;
using this_argument_t = ConditionNode::this_argument_t;
using from_argument_t = ConditionNode::from_argument_t;
using integer_t = ConditionNode::integer_t;
using argument_t = ConditionNode::argument_t;

using no_scope_t = ConditionNode::no_scope_t;
using scope_t = ConditionNode::scope_t;

static constexpr std::string_view THIS_KEYWORD = "THIS";
static constexpr std::string_view FROM_KEYWORD = "FROM";

ConditionNode::ConditionNode(
	Condition const* new_condition,
	argument_t&& new_argument
) : condition { new_condition },
	argument { std::move(new_argument) } {}

bool ConditionNode::execute(
	InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
	scope_t const& from_scope
) const {
	if (condition == nullptr) {
		Logger::error("ConditionNode has no condition!");
		return false;
	}

	return condition->get_execute_callback()(
		instance_manager, current_scope, this_scope, from_scope, argument
	);
}

Condition::Condition(
	std::string_view new_identifier,
	parse_callback_t&& new_parse_callback,
	execute_callback_t&& new_execute_callback
) : HasIdentifier { new_identifier },
	parse_callback { std::move(new_parse_callback) },
	execute_callback { std::move(new_execute_callback) } {}

bool ConditionManager::add_condition(
	std::string_view identifier,
	Condition::parse_callback_t&& parse_callback,
	Condition::execute_callback_t&& execute_callback
) {
	if (identifier.empty()) {
		Logger::error("Invalid condition identifier - empty!");
		return false;
	}

	if (parse_callback == nullptr) {
		Logger::error("Condition ", identifier, " has no parse callback!");
		return false;
	}

	if (execute_callback == nullptr) {
		Logger::error("Condition ", identifier, " has no execute callback!");
		return false;
	}

	return conditions.add_item({
		identifier,
		std::move(parse_callback),
		std::move(execute_callback)
	});
}

ConditionManager::ConditionManager() : root_condition { nullptr } {}

Callback<Condition const&, ast::NodeCPtr> auto ConditionManager::expect_condition_node(
	DefinitionManager const& definition_manager, scope_type_t current_scope, scope_type_t this_scope,
	scope_type_t from_scope, Callback<ConditionNode&&> auto callback
) const {
	return [&definition_manager, current_scope, this_scope, from_scope, callback](
		Condition const& condition, ast::NodeCPtr node
	) -> bool {
		return condition.get_parse_callback()(
			definition_manager, current_scope, this_scope, from_scope, node,
			[callback, &condition](argument_t&& argument) -> bool {
				return callback(ConditionNode { &condition, std::move(argument) });
			}
		);
	};
}

/* Default callback for top condition scope. */
static bool top_scope_fallback(std::string_view id, ast::NodeCPtr node) {
	/* This is a non-condition key, and so not case-insensitive. */
	if (id == "factor") {
		return true;
	} else {
		Logger::error("Unknown node \"", id, "\" found while parsing conditions!");
		return false;
	}
};

NodeCallback auto ConditionManager::expect_condition_node_list_and_length(
	DefinitionManager const& definition_manager, scope_type_t current_scope, scope_type_t this_scope, scope_type_t from_scope,
	Callback<ConditionNode&&> auto callback, LengthCallback auto length_callback, bool top_scope
) const {
	return conditions.expect_item_dictionary_and_length_and_default(
		std::move(length_callback),
		top_scope ? top_scope_fallback : key_value_invalid_callback,
		expect_condition_node(definition_manager, current_scope, this_scope, from_scope, std::move(callback))
	);
}

NodeCallback auto ConditionManager::expect_condition_node_list(
	DefinitionManager const& definition_manager, scope_type_t current_scope, scope_type_t this_scope, scope_type_t from_scope,
	Callback<ConditionNode&&> auto callback, bool top_scope
) const {
	return expect_condition_node_list_and_length(
		definition_manager, current_scope, this_scope, from_scope, std::move(callback), default_length_callback, top_scope
	);
}

node_callback_t ConditionManager::expect_condition_script(
	DefinitionManager const& definition_manager, scope_type_t initial_scope, scope_type_t this_scope,
	scope_type_t from_scope, callback_t<ConditionNode&&> callback
) const {
	return [this, &definition_manager, initial_scope, this_scope, from_scope, callback](ast::NodeCPtr node) -> bool {
		if (root_condition != nullptr) {
			return expect_condition_node(
				definition_manager,
				initial_scope,
				this_scope,
				from_scope,
				callback
			)(*root_condition, node);
		} else {
			Logger::error("Cannot parse condition script: root condition not set!");
			return false;
		}
	};
}

bool ConditionManager::setup_conditions(DefinitionManager const& definition_manager) {
	if (root_condition != nullptr || !conditions_empty()) {
		Logger::error("Cannot set up conditions - root condition is not null and/or condition registry is not empty!");
		return false;
	}

	bool ret = true;

	// TODO - register all conditions here with parsing and execution callbacks

	if (
		add_condition(
			"root condition",
			[](
				DefinitionManager const& definition_manager, scope_type_t current_scope, scope_type_t this_scope,
				scope_type_t from_scope, ast::NodeCPtr node, callback_t<ConditionNode::argument_t&&> callback
			) -> bool {
				return true;
			},
			[](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope, argument_t const& argument
			) -> bool {
				Logger::error("Condition execution not yet implemented!");
				return false;
			}
		)
	) {
		root_condition = &get_conditions().back();
	} else {
		Logger::error("Failed to set root condition! Will not be able to parse condition scripts!");
		ret = false;
	}

	lock_conditions();

	return ret;
}
