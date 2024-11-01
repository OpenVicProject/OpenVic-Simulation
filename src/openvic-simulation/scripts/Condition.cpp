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

// PARSE CALLBACK HELPERS

// If CHANGE_SCOPE is NO_SCOPE then current_scope is propagated through, otherwise the scope changes to CHANGE_SCOPE
// or this_scope/from_scope if CHANGE_SCOPE is THIS or FROM
template<scope_type_t CHANGE_SCOPE, scope_type_t ALLOWED_SCOPES, bool TOP_SCOPE>
bool ConditionManager::_parse_condition_node_list_callback(
	DefinitionManager const& definition_manager, scope_type_t current_scope, scope_type_t this_scope,
	scope_type_t from_scope, ast::NodeCPtr node, callback_t<argument_t&&> callback
) {
	using enum scope_type_t;

	if (!share_scope_type(current_scope, ALLOWED_SCOPES & ALL_SCOPES)) {
		Logger::error(
			"Condition scope mismatch for condition node list - expected ", ALLOWED_SCOPES, ", got ", current_scope
		);
		return false;
	}

	ConditionManager const& condition_manager = definition_manager.get_script_manager().get_condition_manager();

	std::vector<ConditionNode> children;

	const scope_type_t new_scope =
		CHANGE_SCOPE == NO_SCOPE ? current_scope :
		CHANGE_SCOPE == THIS ? this_scope :
		CHANGE_SCOPE == FROM ? from_scope :
		CHANGE_SCOPE;

	if (new_scope == NO_SCOPE) {
		Logger::error(
			"Invalid scope change for condition node list - went from ", current_scope, " to ", new_scope,
			" based on change scope ", CHANGE_SCOPE, " and this/from scope ", this_scope, "/", from_scope
		);
		return false;
	}

	bool ret = condition_manager.expect_condition_node_list_and_length(
		definition_manager, new_scope, this_scope, from_scope,
		vector_callback(children), reserve_length_callback(children), TOP_SCOPE
	)(node);

	ret &= callback(std::move(children));

	return ret;
}

// ALLOWED_SCOPES is a bitfield indicating valid values of current_scope, as well as whether the value is allowed to be
// THIS or FROM corresponding to the special argument types this_argument_t and from_argument_t respectively.
template<typename T, scope_type_t ALLOWED_SCOPES = scope_type_t::ALL_SCOPES>
static bool _parse_condition_node_value_callback(
	DefinitionManager const& definition_manager, scope_type_t current_scope, scope_type_t this_scope,
	scope_type_t from_scope, ast::NodeCPtr node, callback_t<argument_t&&> callback
) {
	static_assert(
		// Value arguments
		std::same_as<T, bool> || std::same_as<T, std::string> || std::same_as<T, integer_t> ||
		std::same_as<T, fixed_point_t> ||

		// Game object arguments
		std::same_as<T, CountryDefinition const*> || std::same_as<T, ProvinceDefinition const*> ||
		std::same_as<T, GoodDefinition const*> || std::same_as<T, Continent const*>
	);

	using enum scope_type_t;

	if (!share_scope_type(current_scope, ALLOWED_SCOPES & ALL_SCOPES)) {
		Logger::error(
			"Condition scope mismatch for ", typeid(T).name(), " value - expected ", ALLOWED_SCOPES, ", got ", current_scope
		);
		return false;
	}

	bool ret = true;

	// All possible value types can also be interpreted as an identifier or string, so we shouldn't get any unwanted error
	// messages if the value is a regular value rather than THIS or FROM. In fact if expect_identifier_or_string returns false
	// when checking for THIS or FROM then we can be confident that it would also return false when parsing a regular value.

	if constexpr (share_scope_type(ALLOWED_SCOPES, THIS | FROM)) {
		std::string_view str;

		ret &= expect_identifier_or_string(assign_variable_callback(str))(node);

		if (!ret) {
			Logger::error("Failed to parse identifier or string when checking for THIS and/or FROM condition argument!");
			return false;
		}

		if constexpr (share_scope_type(ALLOWED_SCOPES, THIS)) {
			if (StringUtils::strings_equal_case_insensitive(str, THIS_KEYWORD)) {
				ret &= callback(this_argument_t {});
				return ret;
			}
		}

		if constexpr (share_scope_type(ALLOWED_SCOPES, FROM)) {
			if (StringUtils::strings_equal_case_insensitive(str, FROM_KEYWORD)) {
				ret &= callback(from_argument_t {});
				return ret;
			}
		}
	}

	T value {};

	if constexpr (std::same_as<T, bool>) {
		ret = expect_bool(assign_variable_callback(value))(node);
	} else if constexpr (std::same_as<T, std::string>) {
		ret = expect_identifier_or_string(assign_variable_callback_string(value))(node);
	} else if constexpr (std::same_as<T, integer_t>) {
		ret = expect_int(assign_variable_callback(value))(node);
	} else if constexpr (std::same_as<T, fixed_point_t>) {
		ret = expect_fixed_point(assign_variable_callback(value))(node);
	} else if constexpr (std::same_as<T, CountryDefinition const*>) {
		ret = definition_manager.get_country_definition_manager().expect_country_definition_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, ProvinceDefinition const*>) {
		ret = definition_manager.get_map_definition().expect_province_definition_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, GoodDefinition const*>) {
		ret =
			definition_manager.get_economy_manager().get_good_definition_manager().expect_good_definition_identifier_or_string(
				assign_variable_callback_pointer(value)
			)(node);
	} else if constexpr (std::same_as<T, Continent const*>) {
		ret = definition_manager.get_map_definition().expect_continent_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	}

	if (ret) {
		ret &= callback(std::move(value));
	}

	return ret;
}

// EXECUTE CALLBACK HELPERS

template<typename RETURN_TYPE, typename SCOPE_TYPE, typename... Args>
static constexpr auto _execute_condition_node_try_cast_scope(
	Functor<
		// return_type(instance_manager, cast_scope, args...)
		RETURN_TYPE, InstanceManager const&, SCOPE_TYPE const&, Args...
	> auto success_callback,
	Functor<
		// return_type(instance_manager, first_scope, args...)
		RETURN_TYPE, InstanceManager const&, scope_t const&, Args...
	> auto failure_callback
) {
	return [success_callback, failure_callback](
		InstanceManager const& instance_manager, scope_t const& first_scope, Args... args
	) -> RETURN_TYPE {
		SCOPE_TYPE const* cast_scope = std::get_if<SCOPE_TYPE>(&first_scope);

		if (cast_scope != nullptr) {
			return success_callback(instance_manager, *cast_scope, args...);
		} else {
			return failure_callback(instance_manager, first_scope, args...);
		}
	};
}

template<typename RETURN_TYPE, typename SCOPE_TYPE, typename... Args>
static constexpr auto _execute_condition_node_cast_scope(
	Functor<
		// return_type(instance_manager, first_scope, args...)
		RETURN_TYPE, InstanceManager const&, SCOPE_TYPE const&, Args...
	> auto callback
) {
	return _execute_condition_node_try_cast_scope<RETURN_TYPE, SCOPE_TYPE, Args...>(
		std::move(callback),
		[](InstanceManager const& instance_manager, scope_t const& first_scope, Args... args) -> RETURN_TYPE {
			Logger::error("Invalid scope for condition node - expected ", typeid(SCOPE_TYPE).name());
			return {};
		}
	);
}

template<typename RETURN_TYPE, typename FIRST_SCOPE_TYPE, typename SECOND_SCOPE_TYPE, typename... Args>
static constexpr auto _execute_condition_node_cast_two_scopes(
	Functor<
		// return_type(instance_manager, first_scope, second_scope, args...)
		RETURN_TYPE, InstanceManager const&, FIRST_SCOPE_TYPE const&, SECOND_SCOPE_TYPE const&, Args...
	> auto callback
) {
	return _execute_condition_node_cast_scope<RETURN_TYPE, FIRST_SCOPE_TYPE, scope_t const&, Args...>(
		[callback](
			InstanceManager const& instance_manager, FIRST_SCOPE_TYPE const& cast_first_scope, scope_t const& second_scope,
			Args... args
		) -> RETURN_TYPE {
			return _execute_condition_node_cast_scope<RETURN_TYPE, SECOND_SCOPE_TYPE, FIRST_SCOPE_TYPE const&, Args...>(
				[callback](
					InstanceManager const& instance_manager, SECOND_SCOPE_TYPE const& cast_second_scope,
					FIRST_SCOPE_TYPE const& cast_first_scope, Args... args
				) -> RETURN_TYPE {
					return callback(instance_manager, cast_first_scope, cast_second_scope, args...);
				}
			)(instance_manager, second_scope, cast_first_scope, args...);
		}
	);
}

// template<typename ARGUMENT_TYPE, typename SCOPE_TYPE, typename... OTHER_SCOPES>
// static constexpr auto _execute_condition_node_try_cast_scope_types(
// 	Callback<
// 		// bool(instance_manager, current_scope, this_scope, from_scope, argument)
// 		InstanceManager const&, SCOPE_TYPE const&, scope_t const&, scope_t const&, ARGUMENT_TYPE const&
// 	> auto callback, auto other_callbacks...
// ) {
// 	return [callback, other_callbacks](
// 		InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
// 		scope_t const& from_scope, ARGUMENT_TYPE const& argument
// 	) -> bool {
// 		return _execute_condition_node_try_cast_scope<
// 			bool, SCOPE_TYPE, scope_t const&, scope_t const&, ARGUMENT_TYPE const&
// 		>(
// 			callback,
// 			[other_callbacks](
// 				InstanceManager const& instance_manager, scope_t const& first_scope, scope_t const& this_scope,
// 				scope_t const& from_scope, ARGUMENT_TYPE const& argument
// 			) -> bool {
// 				if constexpr (sizeof...(OTHER_SCOPES) == 0) {
// 					// TODO - add more detail to this message!
// 					Logger::error("Invalid scope for condition node - expected one from a list of potential types!");
// 				} else {
// 					return _execute_condition_node_try_cast_scope_types<ARGUMENT_TYPE, OTHER_SCOPES...>(
// 						other_callbacks//...
// 					)(instance_manager, first_scope, this_scope, from_scope, argument);
// 				}
// 			}
// 		)(instance_manager, current_scope, this_scope, from_scope, argument);
// 	};
// }

template<typename T, typename... Args>
static constexpr auto _execute_condition_node_try_cast_argument_callback(
	Callback<
		// bool(instance_manager, args..., argument)
		InstanceManager const&, Args..., T const&
	> auto success_callback,
	Callback<
		// bool(instance_manager, args...,)
		InstanceManager const&, Args..., argument_t const&
	> auto failure_callback
) {
	return [success_callback, failure_callback](
		InstanceManager const& instance_manager, Args... args, argument_t const& argument
	) -> bool {
		if constexpr (std::same_as<T, CountryInstance const*>) {
			CountryDefinition const* const* value = std::get_if<CountryDefinition const*>(&argument);

			if (value != nullptr) {
				CountryInstance const* instance =
					&instance_manager.get_country_instance_manager().get_country_instance_from_definition(**value);

				return success_callback(instance_manager, args..., instance);
			}
		} else if constexpr (std::same_as<T, ProvinceInstance const*>) {
			ProvinceDefinition const* const* value = std::get_if<ProvinceDefinition const*>(&argument);

			if (value != nullptr) {
				ProvinceInstance const* instance =
					&instance_manager.get_map_instance().get_province_instance_from_definition(**value);

				return success_callback(instance_manager, args..., instance);
			}
		} else {
			T const* value = std::get_if<T>(&argument);

			if (value != nullptr) {
				return success_callback(instance_manager, args..., *value);
			}
		}

		return failure_callback(instance_manager, args..., argument);
	};
}

template<typename T, typename... Args>
static constexpr auto _execute_condition_node_cast_argument_callback(
	Callback<
		// bool(instance_manager, args..., argument)
		InstanceManager const&, Args..., T const&
	> auto callback
) {
	return _execute_condition_node_try_cast_argument_callback<T, Args...>(
		std::move(callback),
		[](InstanceManager const& instance_manager, Args... args, argument_t const& argument) -> bool {
			Logger::error("ConditionNode missing ", typeid(T).name(), " argument!");
			return false;
		}
	);
}

template<typename T>
static constexpr auto _execute_condition_node_value_or_this_or_from_callback(
	Callback<
		// bool(instance_manager, current_scope, value)
		InstanceManager const&, scope_t const&, T const&
	> auto value_callback,
	Callback<
		// bool(instance_manager, current_scope, this_scope)
		InstanceManager const&, scope_t const&, scope_t const&
	> auto this_callback,
	Callback<
		// bool(instance_manager, current_scope, from_scope)
		InstanceManager const&, scope_t const&, scope_t const&
	> auto from_callback
) {
	return _execute_condition_node_try_cast_argument_callback<T, scope_t const&, scope_t const&, scope_t const&>(
		[value_callback](
			InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
			scope_t const& from_scope, T const& value
		) -> bool {
			return value_callback(instance_manager, current_scope, value);
		},
		[this_callback, from_callback](
			InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
			scope_t const& from_scope, argument_t const& argument
		) -> bool {
			if (ConditionNode::is_this_argument(argument)) {
				return this_callback(instance_manager, current_scope, this_scope);
			}

			if (ConditionNode::is_from_argument(argument)) {
				return from_callback(instance_manager, current_scope, this_scope);
			}

			Logger::error("ConditionNode missing ", typeid(T).name(), " or THIS or FROM argument!");
			return false;
		}
	);
}

template<typename T>
static constexpr auto _execute_condition_node_value_or_cast_this_or_from_callback(
	Callback<
		// bool(instance_manager, current_scope, value)
		InstanceManager const&, scope_t const&, T const&
	> auto callback
) {
	// If IS_THIS is false then the scope is FROM
	const auto cast_scope_callback = [callback]<bool IS_THIS>(
		InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_or_from_scope
	) -> bool {
		T const* cast_this_or_from_scope = std::get_if<T>(&this_or_from_scope);

		if (cast_this_or_from_scope == nullptr) {
			Logger::error("Invalid ", IS_THIS ? "THIS" : "FROM", " scope for condition node - expected ", typeid(T).name());
			return false;
		}

		return callback(instance_manager, current_scope, *cast_this_or_from_scope);
	};

	return _execute_condition_node_value_or_this_or_from_callback<T>(
		callback,
		[cast_scope_callback](
			InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_or_from_scope
		) -> bool {
			return cast_scope_callback.template operator()<true>(instance_manager, current_scope, this_or_from_scope);
		},
		[cast_scope_callback](
			InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_or_from_scope
		) -> bool {
			return cast_scope_callback.template operator()<false>(instance_manager, current_scope, this_or_from_scope);
		}
	);
}

static constexpr scope_t _change_scope_keep_current_scope(
	InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope, scope_t const& from_scope
) {
	return current_scope;
}

/* - EXPECTED_VALUE = what we want child nodes to evaluate to, e.g. true for AND and OR, false for NOT
 * - REQUIRE_ALL = whether all children must evaluate to expected_value or only one, e.g. true for AND and NOT, false for OR */
template<bool EXPECTED_VALUE, bool REQUIRE_ALL, typename T>
static constexpr bool _execute_iterative(
	std::vector<T> const& items, Callback<T const&> auto item_callback
) {
	for (T const& item : items) {
		if (item_callback(item) == (EXPECTED_VALUE != REQUIRE_ALL)) {
			return !REQUIRE_ALL;
		}
	}

	return REQUIRE_ALL;
}

template<bool EXPECTED_VALUE, bool REQUIRE_ALL>
static constexpr bool _execute_condition_node_list(
	InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
	scope_t const& from_scope, std::vector<ConditionNode> const& condition_nodes
) {
	return _execute_iterative<EXPECTED_VALUE, REQUIRE_ALL>(
		condition_nodes,
		[&instance_manager, &current_scope, &this_scope, &from_scope](ConditionNode const& condition_node) -> bool {
			return condition_node.execute(instance_manager, current_scope, this_scope, from_scope);
		}
	);
}

/* - change_scope = returns the current scope for the child conditions to be executed with */
template<bool EXPECTED_VALUE, bool REQUIRE_ALL>
static constexpr auto _execute_condition_node_list_single_scope_callback(
	Functor<
		// new_scope(instance_manager, current_scope, this_scope, from_scope)
		scope_t, InstanceManager const&, scope_t const&, scope_t const&, scope_t const&
	> auto change_scope
) {
	return [change_scope](
		InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
		scope_t const& from_scope, argument_t const& argument
	) -> bool {
		return _execute_condition_node_cast_argument_callback<
			std::vector<ConditionNode>, scope_t const&, scope_t const&, scope_t const&
		>(
			[change_scope](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope, std::vector<ConditionNode> const& argument
			) -> bool {
				const scope_t new_scope = change_scope(instance_manager, current_scope, this_scope, from_scope);

				if (ConditionNode::is_no_scope(new_scope)) {
					Logger::error("Invalid scope change for condition node list - no scope!");
					return false;
				}

				return _execute_condition_node_list<EXPECTED_VALUE, REQUIRE_ALL>(
					instance_manager, new_scope, this_scope, from_scope, argument
				);
			}
		)(instance_manager, current_scope, this_scope, from_scope, argument);
	};
}

/* - change_scopes = returns the vector of current scopes for the child conditions to be executed with
 * - here EXPECTED_VALUE and REQUIRE_ALL refer to whether the results per scope are expected to be true and if all are needed,
 *   the conditions themselves are all expected to be true and are all required for each scope individually. */
template<bool EXPECTED_VALUE, bool REQUIRE_ALL>
static constexpr auto _execute_condition_node_list_multi_scope_callback(
	Functor<
		// new_scopes(instance_manager, current_scope, this_scope, from_scope)
		std::vector<scope_t>, InstanceManager const&, scope_t const&, scope_t const&, scope_t const&
	> auto change_scopes
) {
	return [change_scopes](
		InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
		scope_t const& from_scope, argument_t const& argument
	) -> bool {
		return _execute_condition_node_cast_argument_callback<
			std::vector<ConditionNode>, scope_t const&, scope_t const&, scope_t const&
		>(
			[change_scopes](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope, std::vector<ConditionNode> const& argument
			) -> bool {
				return _execute_iterative<EXPECTED_VALUE, REQUIRE_ALL>(
					change_scopes(instance_manager, current_scope, this_scope, from_scope),
					[&instance_manager, &this_scope, &from_scope, &argument](scope_t const& new_scope) -> bool {
						return _execute_condition_node_list<true, true>(
							instance_manager, new_scope, this_scope, from_scope, argument
						);
					}
				);
			}
		)(instance_manager, current_scope, this_scope, from_scope, argument);
	};
}

bool ConditionManager::setup_conditions(DefinitionManager const& definition_manager) {
	if (root_condition != nullptr || !conditions_empty()) {
		Logger::error("Cannot set up conditions - root condition is not null and/or condition registry is not empty!");
		return false;
	}

	using enum scope_type_t;

	static constexpr scope_type_t no_scope_change = NO_SCOPE;
	static constexpr scope_type_t all_scopes_allowed = ALL_SCOPES;
	static constexpr bool top_scope = true;
	static constexpr bool expect_true = true;
	static constexpr bool expect_false = false;
	static constexpr bool require_all = true;
	static constexpr bool require_any = false;

	bool ret = true;

	/* Special Scopes */
	ret &= add_condition(
		THIS_KEYWORD,
		_parse_condition_node_list_callback<THIS>,
		_execute_condition_node_list_single_scope_callback<expect_true, require_all>(
			[](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope
			) -> scope_t {
				return this_scope;
			}
		)
	);
	ret &= add_condition(
		FROM_KEYWORD,
		_parse_condition_node_list_callback<FROM>,
		_execute_condition_node_list_single_scope_callback<expect_true, require_all>(
			[](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope
			) -> scope_t {
				return from_scope;
			}
		)
	);
	// ret &= add_condition("independence", GROUP, COUNTRY, COUNTRY); //only from rebels!

	/* Trigger Country Scopes */
	static const auto get_core_scopes =
		_execute_condition_node_cast_scope<std::vector<scope_t>, CountryInstance const*, scope_t const&, scope_t const&>(
			[](
				InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
				scope_t const& this_scope, scope_t const& from_scope
			) -> std::vector<scope_t> {
				ordered_set<ProvinceInstance*> const& core_provinces = current_scope->get_core_provinces();

				std::vector<scope_t> core_province_scopes;
				core_province_scopes.reserve(core_provinces.size());

				for (ProvinceInstance const* core_province : core_provinces) {
					core_province_scopes.push_back(core_province);
				}

				return core_province_scopes;
			}
		);
	ret &= add_condition(
		"all_core",
		_parse_condition_node_list_callback<PROVINCE, COUNTRY>,
		_execute_condition_node_list_multi_scope_callback<expect_true, require_all>(get_core_scopes)
	);
	ret &= add_condition(
		"any_core",
		_parse_condition_node_list_callback<PROVINCE, COUNTRY>,
		_execute_condition_node_list_multi_scope_callback<expect_true, require_any>(get_core_scopes)
	);
	ret &= add_condition(
		"any_greater_power", // Great powers, doesn't include secondary powers
		_parse_condition_node_list_callback<COUNTRY>,
		_execute_condition_node_list_multi_scope_callback<expect_true, require_any>(
			[](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope
			) -> std::vector<scope_t> {
				std::vector<CountryInstance*> const& great_powers =
					instance_manager.get_country_instance_manager().get_great_powers();

				std::vector<scope_t> great_power_scopes;
				great_power_scopes.reserve(great_powers.size());

				for (CountryInstance const* great_power : great_powers) {
					great_power_scopes.push_back(great_power);
				}

				return great_power_scopes;
			}
		)
	);
	ret &= add_condition(
		"any_neighbor_country",
		_parse_condition_node_list_callback<COUNTRY, COUNTRY>,
		_execute_condition_node_list_multi_scope_callback<expect_true, require_any>(
			_execute_condition_node_cast_scope<std::vector<scope_t>, CountryInstance const*, scope_t const&, scope_t const&>(
				[](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope
				) -> std::vector<scope_t> {
					std::vector<scope_t> neighbouring_country_scopes;

					// TODO - fill neighbouring_country_scopes with pointers to countries neighbouring *current_scope

					return neighbouring_country_scopes;
				}
			)
		)
	);
	ret &= add_condition(
		"any_owned_province",
		_parse_condition_node_list_callback<PROVINCE, COUNTRY>,
		_execute_condition_node_list_multi_scope_callback<expect_true, require_any>(
			_execute_condition_node_cast_scope<std::vector<scope_t>, CountryInstance const*, scope_t const&, scope_t const&>(
				[](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope
				) -> std::vector<scope_t> {
					ordered_set<ProvinceInstance*> const& owned_provinces = current_scope->get_owned_provinces();

					std::vector<scope_t> owned_province_scopes;
					owned_province_scopes.reserve(owned_provinces.size());

					for (ProvinceInstance const* owned_province : owned_provinces) {
						owned_province_scopes.push_back(owned_province);
					}

					return owned_province_scopes;
				}
			)
		)
	);
	ret &= add_condition(
		"any_pop",
		_parse_condition_node_list_callback<POP, COUNTRY | PROVINCE>,
		_execute_condition_node_list_multi_scope_callback<expect_true, require_any>(
			[](
				InstanceManager const& instance_manager, scope_t const& current_scope,
				scope_t const& this_scope, scope_t const& from_scope
			) -> std::vector<scope_t> {
				// TODO - fill with all pops in current_scope (either a country or a province)
				return {};
			}
		)
	);
	// ret &= add_condition("any_sphere_member", GROUP, COUNTRY, COUNTRY);
	// ret &= add_condition("any_state", GROUP, COUNTRY, STATE);
	// ret &= add_condition("any_substate", GROUP, COUNTRY, COUNTRY);
	ret &= add_condition(
		"capital_scope",
		_parse_condition_node_list_callback<PROVINCE, COUNTRY>,
		_execute_condition_node_list_single_scope_callback<expect_true, require_all>(
			_execute_condition_node_cast_scope<scope_t, CountryInstance const*, scope_t const&, scope_t const&>(
				[](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope
				) -> scope_t {
					ProvinceInstance const* capital = current_scope->get_capital();

					if (capital == nullptr) {
						Logger::error("Cannot create province scope for capital_scope condition - country has no capital!");
						return no_scope_t {};
					}

					return capital;
				}
			)
		)
	);
	// ret &= add_condition("country", GROUP, COUNTRY, COUNTRY);
	// ret &= add_condition("cultural_union", GROUP, COUNTRY, COUNTRY);
	// ret &= add_condition("overlord", GROUP, COUNTRY, COUNTRY);
	// ret &= add_condition("sphere_owner", GROUP, COUNTRY, COUNTRY);
	// ret &= add_condition("war_countries", GROUP, COUNTRY, COUNTRY);

	/* Trigger State Scopes */
	// ret &= add_condition("any_neighbor_province", GROUP, STATE, PROVINCE);

	/* Trigger Province Scopes */
	ret &= add_condition(
		"controller",
		_parse_condition_node_list_callback<COUNTRY, PROVINCE>,
		_execute_condition_node_list_single_scope_callback<expect_true, require_all>(
			_execute_condition_node_cast_scope<scope_t, ProvinceInstance const*, scope_t const&, scope_t const&>(
				[](
					InstanceManager const& instance_manager, ProvinceInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope
				) -> scope_t {
					CountryInstance const* controller = current_scope->get_controller();

					if (controller == nullptr) {
						Logger::error("Cannot create country scope for controller condition - province has no controller!");
						return no_scope_t {};
					}

					return controller;
				}
			)
		)
	);
	ret &= add_condition(
		"owner",
		_parse_condition_node_list_callback<COUNTRY, PROVINCE>,
		_execute_condition_node_list_single_scope_callback<expect_true, require_all>(
			_execute_condition_node_cast_scope<scope_t, ProvinceInstance const*, scope_t const&, scope_t const&>(
				[](
					InstanceManager const& instance_manager, ProvinceInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope
				) -> scope_t {
					CountryInstance const* owner = current_scope->get_owner();

					if (owner == nullptr) {
						Logger::error("Cannot create country scope for owner condition - province has no owner!");
						return no_scope_t {};
					}

					return owner;
				}
			)
		)
	);
	// ret &= add_condition("state_scope", GROUP, PROVINCE, STATE);

	/* Trigger Pop Scopes */
	ret &= add_condition(
		"location",
		_parse_condition_node_list_callback<PROVINCE, POP>,
		_execute_condition_node_list_single_scope_callback<expect_true, require_all>(
			_execute_condition_node_cast_scope<scope_t, Pop const*, scope_t const&, scope_t const&>(
				[](
					InstanceManager const& instance_manager, Pop const* const& current_scope, scope_t const& this_scope,
					scope_t const& from_scope
				) -> scope_t {
					ProvinceInstance const* location = current_scope->get_location();

					if (location == nullptr) {
						Logger::error("Cannot create province scope for location condition - pop has no location!");
						return no_scope_t {};
					}

					return location;
				}
			)
		)
	);

	/* Special Conditions */
	ret &= add_condition(
		"AND",
		_parse_condition_node_list_callback<>,
		_execute_condition_node_list_single_scope_callback<expect_true, require_all>(_change_scope_keep_current_scope)
	);
	ret &= add_condition(
		"OR",
		_parse_condition_node_list_callback<>,
		_execute_condition_node_list_single_scope_callback<expect_true, require_any>(_change_scope_keep_current_scope)
	);
	ret &= add_condition(
		"NOT",
		_parse_condition_node_list_callback<>,
		_execute_condition_node_list_single_scope_callback<expect_false, require_all>(_change_scope_keep_current_scope)
	);

	/* Global Conditions */
	ret &= add_condition(
		"year",
		_parse_condition_node_value_callback<integer_t>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t const&, scope_t const&, scope_t const&>(
			[](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope, integer_t const& argument
			) -> bool {
				return instance_manager.get_today().get_year() >= argument;
			}
		)
	);
	ret &= add_condition(
		"month",
		_parse_condition_node_value_callback<integer_t>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t const&, scope_t const&, scope_t const&>(
			[](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope, integer_t const& argument
			) -> bool {
				// Month condition values are indexed from 0 and Date months are indexed from 1, so we need to check
				// current month >= condition month + 1. As both values are integers, this is equivalent to:
				return instance_manager.get_today().get_month() > argument;
			}
		)
	);
	ret &= add_condition(
		"has_global_flag",
		_parse_condition_node_value_callback<std::string>,
		_execute_condition_node_cast_argument_callback<std::string, scope_t const&, scope_t const&, scope_t const&>(
			[](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope, std::string const& argument
			) -> bool {
				// TODO - check if global flag "argument" is set
				return false;
			}
		)
	);
	ret &= add_condition(
		"is_canal_enabled",
		_parse_condition_node_value_callback<integer_t>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t const&, scope_t const&, scope_t const&>(
			[](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope, integer_t const& argument
			) -> bool {
				// TODO - check if canal[argument] is enabled
				return false;
			}
		)
	);
	ret &= add_condition(
		"always",
		_parse_condition_node_value_callback<bool>,
		_execute_condition_node_cast_argument_callback<bool, scope_t const&, scope_t const&, scope_t const&>(
			[](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope, bool const& argument
			) -> bool {
				return argument;
			}
		)
	);
	ret &= add_condition(
		"world_wars_enabled",
		_parse_condition_node_value_callback<bool>,
		_execute_condition_node_cast_argument_callback<bool, scope_t const&, scope_t const&, scope_t const&>(
			[](
				InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
				scope_t const& from_scope, bool const& argument
			) -> bool {
				// TODO - check if world wars are enabled == argument
				return false;
			}
		)
	);

	/* Country Scope Conditions */
	ret &= add_condition(
		"administration_spending",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t const&, scope_t const&, scope_t const&>(
			_execute_condition_node_cast_scope<
				bool, CountryInstance const*, scope_t const&, scope_t const&, fixed_point_t const&
			>(
				[](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope, fixed_point_t const& argument
				) -> bool {
					// TODO - check if *country has administration spending >= argument (in the range 0 - 1)
					return false;
				}
			)
		)
	);
	ret &= add_condition(
		"ai",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t const&, scope_t const&, scope_t const&>(
			_execute_condition_node_cast_scope<bool, CountryInstance const*, scope_t const&, scope_t const&, bool const&>(
				[](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope, bool const& argument
				) -> bool {
					// TODO - check if *country is ai == argument
					return false;
				}
			)
		)
	);
	ret &= add_condition(
		"alliance_with",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_value_or_cast_this_or_from_callback<CountryInstance const*>(
			_execute_condition_node_cast_scope<bool, CountryInstance const*, CountryInstance const* const&>(
				[](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					CountryInstance const* const& value
				) -> bool {
					// TODO - check if *current_scope and *value have alliance
					return false;
				}
			)
		)
	);
	ret &= add_condition(
		"average_consciousness",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t const&, scope_t const&, scope_t const&>(
			_execute_condition_node_cast_scope<
				bool, CountryInstance const*, scope_t const&, scope_t const&, fixed_point_t const&
			>(
				[](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope, fixed_point_t const& argument
				) -> bool {
					return current_scope->get_national_consciousness() >= argument;
				}
			)
		)

		// TODO - can be used on province too!!!

		// _parse_condition_node_value_callback<fixed_point_t, COUNTRY | PROVINCE>,
		// _execute_condition_node_cast_argument_callback<fixed_point_t, scope_t const&, scope_t const&, scope_t const&>(
		// 	_execute_condition_node_try_cast_scope_types<
		// 		bool, CountryInstance const*, ProvinceInstance const*
		// 	>(
		// 		[](
		// 			InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
		// 			scope_t const& this_scope, scope_t const& from_scope, fixed_point_t const& argument
		// 		) -> bool {
		// 			return current_scope->get_national_consciousness() >= argument;
		// 		},
		// 		[](
		// 			InstanceManager const& instance_manager, ProvinceInstance const* const& current_scope,
		// 			scope_t const& this_scope, scope_t const& from_scope, fixed_point_t const& argument
		// 		) -> bool {
		// 			return current_scope->get_average_consciousness() >= argument;
		// 		},
		// 		[](
		// 			InstanceManager const& instance_manager, ProvinceInstance const* const& current_scope,
		// 			scope_t const& this_scope, scope_t const& from_scope, fixed_point_t const& argument
		// 		) -> bool {
		// 			return false;
		// 		}
		// 	)
		// )
	);
	ret &= add_condition(
		"average_militancy",


		// TODO - can be used on province too!!!


		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t const&, scope_t const&, scope_t const&>(
			_execute_condition_node_cast_scope<
				bool, CountryInstance const*, scope_t const&, scope_t const&, fixed_point_t const&
			>(
				[](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope, fixed_point_t const& argument
				) -> bool {
					return current_scope->get_national_militancy() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"badboy",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t const&, scope_t const&, scope_t const&>(
			_execute_condition_node_cast_scope<
				bool, CountryInstance const*, scope_t const&, scope_t const&, fixed_point_t const&
			>(
				[&definition_manager](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope, fixed_point_t const& argument
				) -> bool {
					// TODO - multiply argument by infamy_containment_limit during parsing rather than during every execution?
					return current_scope->get_infamy() >= argument
						* definition_manager.get_define_manager().get_country_defines().get_infamy_containment_limit();
				}
			)
		)
	);
	ret &= add_condition(
		"big_producer",
		_parse_condition_node_value_callback<GoodDefinition const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<GoodDefinition const*, scope_t const&, scope_t const&, scope_t const&>(
			_execute_condition_node_cast_scope<
				bool, CountryInstance const*, scope_t const&, scope_t const&, GoodDefinition const* const&
			>(
				[&definition_manager](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope, GoodDefinition const* const& argument
				) -> bool {
					// TODO - check if *current_scope is big producer of *argument
					return false;
				}
			)
		)
	);
	ret &= add_condition(
		"blockade",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t const&, scope_t const&, scope_t const&>(
			_execute_condition_node_cast_scope<
				bool, CountryInstance const*, scope_t const&, scope_t const&, fixed_point_t const&
			>(
				[&definition_manager](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope, fixed_point_t const& argument
				) -> bool {
					// TODO - check if proportion of *current_scope's ports that are blockaded is >= argument
					return false;
				}
			)
		)
	);
	// ret &= add_condition("brigades_compare", REAL, COUNTRY);
	// ret &= add_condition("can_build_factory_in_capital_state", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, BUILDING);
	// ret &= add_condition("can_build_fort_in_capital", COMPLEX, COUNTRY);
	// ret &= add_condition("can_build_railway_in_capital", COMPLEX, COUNTRY);
	// ret &= add_condition("can_nationalize", BOOLEAN, COUNTRY);
	// ret &= add_condition("can_create_vassals", BOOLEAN, COUNTRY);
	// ret &= add_condition("capital", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, PROVINCE_ID);
	// ret &= add_condition("casus_belli", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("check_variable", COMPLEX, COUNTRY, NO_SCOPE, NO_IDENTIFIER, VARIABLE);
	// ret &= add_condition("citizenship_policy", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	// ret &= add_condition("civilization_progress", REAL, COUNTRY);
	ret &= add_condition(
		"civilized",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t const&, scope_t const&, scope_t const&>(
			_execute_condition_node_cast_scope<bool, CountryInstance const*, scope_t const&, scope_t const&, bool const&>(
				[](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope, bool const& argument
				) -> bool {
					return current_scope->is_civilised() == argument;
				}
			)
		)
	);
	// ret &= add_condition("colonial_nation", BOOLEAN, COUNTRY);
	// ret &= add_condition("consciousness", REAL, COUNTRY);
	// ret &= add_condition("constructing_cb_progress", REAL, COUNTRY);
	// ret &= add_condition("constructing_cb_type", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CASUS_BELLI);
	ret &= add_condition("continent",
		_parse_condition_node_value_callback<Continent const*, PROVINCE>,
		_execute_condition_node_cast_argument_callback<Continent const*, scope_t const&, scope_t const&, scope_t const&>(
			_execute_condition_node_cast_scope<
				bool, ProvinceInstance const*, scope_t const&, scope_t const&, Continent const* const&
			>(
				[](
					InstanceManager const& instance_manager, ProvinceInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope, Continent const* const& argument
				) -> bool {
					return current_scope->get_province_definition().get_continent() == argument;
				}
			)
		)
	);
	// ret &= add_condition("controls", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, PROVINCE_ID);
	// ret &= add_condition("crime_fighting", REAL, COUNTRY);
	// ret &= add_condition("crime_higher_than_education", BOOLEAN, COUNTRY);
	// ret &= add_condition("crisis_exist", BOOLEAN, COUNTRY);
	// ret &= add_condition("culture", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE);
	// ret &= add_condition("culture_has_union_tag", BOOLEAN, COUNTRY);
	// ret &= add_condition("diplomatic_influence", COMPLEX, COUNTRY);
	// ret &= add_condition("economic_policy", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	// ret &= add_condition("education_spending", REAL, COUNTRY);
	// ret &= add_condition("election", BOOLEAN, COUNTRY);
	// ret &= add_condition("exists", IDENTIFIER | BOOLEAN, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("government", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, GOVERNMENT_TYPE);
	// ret &= add_condition("great_wars_enabled", BOOLEAN, COUNTRY);
	// ret &= add_condition("have_core_in", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition(
		"has_country_flag",
		_parse_condition_node_value_callback<std::string, COUNTRY>,
		_execute_condition_node_cast_argument_callback<std::string, scope_t const&, scope_t const&, scope_t const&>(
			_execute_condition_node_cast_scope<
				bool, CountryInstance const*, scope_t const&, scope_t const&, std::string const&
			>(
				[&definition_manager](
					InstanceManager const& instance_manager, CountryInstance const* const& current_scope,
					scope_t const& this_scope, scope_t const& from_scope, std::string const& argument
				) -> bool {
					return current_scope->has_country_flag(argument);
				}
			)
		)
	);
	// ret &= add_condition("has_country_modifier", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_EVENT_MODIFIER);
	// ret &= add_condition("has_cultural_sphere", BOOLEAN, COUNTRY);
	// ret &= add_condition("has_leader", STRING, COUNTRY);
	// ret &= add_condition("has_pop_culture", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE);
	// ret &= add_condition("has_pop_religion", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, RELIGION);
	// ret &= add_condition("has_pop_type", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, POP_TYPE);
	// ret &= add_condition("has_recently_lost_war", BOOLEAN, COUNTRY);
	// ret &= add_condition("has_unclaimed_cores", BOOLEAN, COUNTRY);
	// ret &= add_condition("ideology", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, IDEOLOGY);
	// ret &= add_condition("industrial_score", REAL | IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("in_sphere", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("in_default", IDENTIFIER | BOOLEAN, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("invention", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, INVENTION);
	// ret &= add_condition("involved_in_crisis", BOOLEAN, COUNTRY);
	// ret &= add_condition("is_claim_crisis", BOOLEAN, COUNTRY);
	// ret &= add_condition("is_colonial_crisis", BOOLEAN, COUNTRY);
	// ret &= add_condition("is_cultural_union", IDENTIFIER | BOOLEAN, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("is_disarmed", BOOLEAN, COUNTRY);
	// ret &= add_condition("is_greater_power", BOOLEAN, COUNTRY);
	// ret &= add_condition("is_colonial", BOOLEAN, STATE);
	// ret &= add_condition("is_core", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG | PROVINCE_ID);
	// ret &= add_condition("is_culture_group", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG | CULTURE_GROUP);
	// ret &= add_condition("is_ideology_enabled", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, IDEOLOGY);
	// ret &= add_condition("is_independant", BOOLEAN, COUNTRY); //paradox typo
	// ret &= add_condition("is_liberation_crisis", BOOLEAN, COUNTRY);
	// ret &= add_condition("is_mobilised", BOOLEAN, COUNTRY);
	// ret &= add_condition("is_next_reform", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, REFORM);
	// ret &= add_condition("is_our_vassal", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("is_possible_vassal", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("is_releasable_vassal", IDENTIFIER | BOOLEAN, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("is_secondary_power", BOOLEAN, COUNTRY);
	// ret &= add_condition("is_sphere_leader_of", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("is_substate", BOOLEAN, COUNTRY);
	// ret &= add_condition("is_vassal", BOOLEAN, COUNTRY);
	// ret &= add_condition("literacy", REAL, COUNTRY);
	// ret &= add_condition("lost_national", REAL, COUNTRY);
	// ret &= add_condition("middle_strata_militancy", REAL, COUNTRY);
	// ret &= add_condition("middle_strata_everyday_needs", REAL, COUNTRY);
	// ret &= add_condition("middle_strata_life_needs", REAL, COUNTRY);
	// ret &= add_condition("middle_strata_luxury_needs", REAL, COUNTRY);
	// ret &= add_condition("middle_tax", REAL, COUNTRY);
	// ret &= add_condition("military_access", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("military_score", REAL | IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("militancy", REAL, COUNTRY);
	// ret &= add_condition("military_spending", REAL, COUNTRY);
	// ret &= add_condition("money", REAL, COUNTRY);
	// ret &= add_condition("nationalvalue", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, NATIONAL_VALUE);
	// ret &= add_condition("national_provinces_occupied", REAL, COUNTRY);
	// ret &= add_condition("neighbour", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("num_of_allies", INTEGER, COUNTRY);
	// ret &= add_condition("num_of_cities", INTEGER, COUNTRY);
	// ret &= add_condition("num_of_ports", INTEGER, COUNTRY);
	// ret &= add_condition("num_of_revolts", INTEGER, COUNTRY);
	// ret &= add_condition("number_of_states", INTEGER, COUNTRY);
	// ret &= add_condition("num_of_substates", INTEGER, COUNTRY);
	// ret &= add_condition("num_of_vassals", INTEGER, COUNTRY);
	// ret &= add_condition("num_of_vassals_no_substates", INTEGER, COUNTRY);
	// ret &= add_condition("owns", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, PROVINCE_ID);
	// ret &= add_condition("part_of_sphere", BOOLEAN, COUNTRY);
	// ret &= add_condition("plurality", REAL, COUNTRY);
	// ret &= add_condition("political_movement_strength", REAL, COUNTRY);
	// ret &= add_condition("political_reform_want", REAL, COUNTRY);
	// ret &= add_condition("pop_majority_culture", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE);
	// ret &= add_condition("pop_majority_ideology", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, IDEOLOGY);
	// ret &= add_condition("pop_majority_religion", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, RELIGION);
	// ret &= add_condition("pop_militancy", REAL, COUNTRY);
	// ret &= add_condition("poor_strata_militancy", REAL, COUNTRY);
	// ret &= add_condition("poor_strata_everyday_needs", REAL, COUNTRY);
	// ret &= add_condition("poor_strata_life_needs", REAL, COUNTRY);
	// ret &= add_condition("poor_strata_luxury_needs", REAL, COUNTRY);
	// ret &= add_condition("poor_tax", REAL, COUNTRY);
	// ret &= add_condition("prestige", REAL, COUNTRY);
	// ret &= add_condition("primary_culture", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE);
	// ret &= add_condition("accepted_culture", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE);
	// ret &= add_condition("produces", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, TRADE_GOOD);
	// ret &= add_condition("rank", INTEGER, COUNTRY);
	// ret &= add_condition("rebel_power_fraction", REAL, COUNTRY);
	// ret &= add_condition("recruited_percentage", REAL, COUNTRY);
	// ret &= add_condition("relation", COMPLEX, COUNTRY);
	// ret &= add_condition("religion", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, RELIGION);
	// ret &= add_condition("religious_policy", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	// ret &= add_condition("revanchism", REAL, COUNTRY);
	// ret &= add_condition("revolt_percentage", REAL, COUNTRY);
	// ret &= add_condition("rich_strata_militancy", REAL, COUNTRY);
	// ret &= add_condition("rich_strata_everyday_needs", REAL, COUNTRY);
	// ret &= add_condition("rich_strata_life_needs", REAL, COUNTRY);
	// ret &= add_condition("rich_strata_luxury_needs", REAL, COUNTRY);
	// ret &= add_condition("rich_tax", REAL, COUNTRY);
	// ret &= add_condition("rich_tax_above_poor", BOOLEAN, COUNTRY);
	// ret &= add_condition("ruling_party", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("ruling_party_ideology", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, IDEOLOGY);
	// ret &= add_condition("social_movement_strength", REAL, COUNTRY);
	// ret &= add_condition("social_reform_want", REAL, COUNTRY);
	// ret &= add_condition("social_spending", REAL, COUNTRY);
	// ret &= add_condition("stronger_army_than", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("substate_of", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("tag", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("tech_school", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, TECH_SCHOOL);
	// ret &= add_condition("this_culture_union", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE_UNION);
	// ret &= add_condition("total_amount_of_divisions", INTEGER, COUNTRY);
	// ret &= add_condition("total_amount_of_ships", INTEGER, COUNTRY);
	// ret &= add_condition("total_defensives", INTEGER, COUNTRY);
	// ret &= add_condition("total_num_of_ports", INTEGER, COUNTRY);
	// ret &= add_condition("total_of_ours_sunk", INTEGER, COUNTRY);
	// ret &= add_condition("total_pops", INTEGER, COUNTRY);
	// ret &= add_condition("total_sea_battles", INTEGER, COUNTRY);
	// ret &= add_condition("total_sunk_by_us", INTEGER, COUNTRY);
	// ret &= add_condition("trade_policy", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	// ret &= add_condition("treasury", REAL, COUNTRY);
	// ret &= add_condition("truce_with", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("unemployment", REAL, COUNTRY);
	// ret &= add_condition("unit_has_leader", BOOLEAN, COUNTRY);
	// ret &= add_condition("unit_in_battle", BOOLEAN, COUNTRY);
	// ret &= add_condition("upper_house", COMPLEX, COUNTRY);
	// ret &= add_condition("vassal_of", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("war", BOOLEAN, COUNTRY);
	// ret &= add_condition("war_exhaustion", REAL, COUNTRY);
	// ret &= add_condition("war_policy", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	// ret &= add_condition("war_score", REAL, COUNTRY);
	// ret &= add_condition("war_with", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);

	/* State Scope Conditions */
	// ret &= add_condition("controlled_by", IDENTIFIER, STATE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("empty", BOOLEAN, STATE);
	// ret &= add_condition("flashpoint_tension", REAL, STATE);
	// ret &= add_condition("has_building", IDENTIFIER, STATE, NO_SCOPE, NO_IDENTIFIER, BUILDING);
	// ret &= add_condition("has_factories", BOOLEAN, STATE);
	// ret &= add_condition("has_flashpoint", BOOLEAN, STATE);
	// ret &= add_condition("is_slave", BOOLEAN, STATE);
	// ret &= add_condition("owned_by", IDENTIFIER, STATE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("trade_goods_in_state", IDENTIFIER, STATE, NO_SCOPE, NO_IDENTIFIER, TRADE_GOOD);
	// ret &= add_condition("work_available", COMPLEX, STATE);

	/* Province Scope Conditions */
	// ret &= add_condition("can_build_factory", BOOLEAN, PROVINCE);
	// ret &= add_condition("controlled_by_rebels", BOOLEAN, PROVINCE);
	// ret &= add_condition("country_units_in_province", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("country_units_in_state", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("has_crime", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, CRIME);
	// ret &= add_condition("has_culture_core", BOOLEAN, PROVINCE);
	// ret &= add_condition("has_empty_adjacent_province", BOOLEAN, PROVINCE);
	// ret &= add_condition("has_empty_adjacent_state", BOOLEAN, PROVINCE);
	// ret &= add_condition("has_national_minority", BOOLEAN, PROVINCE);
	// ret &= add_condition("has_province_flag", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, PROVINCE_FLAG);
	// ret &= add_condition("has_province_modifier", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, PROVINCE_EVENT_MODIFIER);
	// ret &= add_condition("has_recent_imigration", INTEGER, PROVINCE); //paradox typo
	// ret &= add_condition("is_blockaded", BOOLEAN, PROVINCE);
	// ret &= add_condition("is_accepted_culture", IDENTIFIER | BOOLEAN, PROVINCE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("is_capital", BOOLEAN, PROVINCE);
	// ret &= add_condition("is_coastal", BOOLEAN, PROVINCE);
	// ret &= add_condition("is_overseas", BOOLEAN, PROVINCE);
	// ret &= add_condition("is_primary_culture", IDENTIFIER | BOOLEAN, PROVINCE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	// ret &= add_condition("is_state_capital", BOOLEAN, PROVINCE);
	// ret &= add_condition("is_state_religion", BOOLEAN, PROVINCE);
	// ret &= add_condition("life_rating", REAL, PROVINCE);
	// ret &= add_condition("minorities", BOOLEAN, PROVINCE);
	// ret &= add_condition("port", BOOLEAN, PROVINCE);
	// ret &= add_condition("province_control_days", INTEGER, PROVINCE);
	// ret &= add_condition("province_id", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, PROVINCE_ID);
	// ret &= add_condition("region", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, REGION);
	// ret &= add_condition("state_id", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, PROVINCE_ID);
	// ret &= add_condition("terrain", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, TERRAIN);
	// ret &= add_condition("trade_goods", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, TRADE_GOOD);
	// ret &= add_condition("unemployment_by_type", COMPLEX, PROVINCE);
	// ret &= add_condition("units_in_province", INTEGER, PROVINCE);

	/* Pop Scope Conditions */
	// ret &= add_condition("agree_with_ruling_party", REAL, POP);
	// ret &= add_condition("cash_reserves", REAL, POP);
	// ret &= add_condition("everyday_needs", REAL, POP);
	// ret &= add_condition("life_needs", REAL, POP);
	// ret &= add_condition("luxury_needs", REAL, POP);
	// ret &= add_condition("political_movement", BOOLEAN, POP);
	// ret &= add_condition("pop_majority_issue", IDENTIFIER, POP, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	// ret &= add_condition("pop_type", IDENTIFIER, POP, NO_SCOPE, NO_IDENTIFIER, POP_TYPE);
	// ret &= add_condition("social_movement", BOOLEAN, POP);
	// ret &= add_condition("strata", IDENTIFIER, POP, NO_SCOPE, NO_IDENTIFIER, POP_STRATA);
	// ret &= add_condition("type", IDENTIFIER, POP, NO_SCOPE, NO_IDENTIFIER, POP_TYPE);

	// const auto import_identifiers = [this, &ret](
	// 	std::vector<std::string_view> const& identifiers,
	// 	value_type_t value_type,
	// 	scope_type_t scope,
	// 	scope_type_t scope_change = NO_SCOPE,
	// 	identifier_type_t key_identifier_type = NO_IDENTIFIER,
	// 	identifier_type_t value_identifier_type = NO_IDENTIFIER
	// ) -> void {
	// 	for (std::string_view const& identifier : identifiers) {
	// 		ret &= add_condition(
	// 			identifier, value_type, scope, scope_change,
	// 			key_identifier_type, value_identifier_type
	// 		);
	// 	}
	// };

	/* Scopes from other registries */
	// import_identifiers(
	// 	definition_manager.get_country_definition_manager().get_country_definition_identifiers(),
	// 	GROUP,
	// 	COUNTRY,
	// 	COUNTRY,
	// 	COUNTRY_TAG,
	// 	NO_IDENTIFIER
	// );
	for (CountryDefinition const& country : definition_manager.get_country_definition_manager().get_country_definitions()) {
		ret &= add_condition(
			country.get_identifier(),
			_parse_condition_node_list_callback<COUNTRY>,
			_execute_condition_node_list_single_scope_callback<expect_true, require_all>(
				[&country](
					InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
					scope_t const& from_scope
				) -> scope_t {
					return &instance_manager.get_country_instance_manager().get_country_instance_from_definition(country);
				}
			)
		);
	}

	// import_identifiers(
	// 	definition_manager.get_map_definition().get_region_identifiers(),
	// 	GROUP,
	// 	COUNTRY,
	// 	STATE,
	// 	REGION,
	// 	NO_IDENTIFIER
	// );

	// import_identifiers(
	// 	definition_manager.get_map_definition().get_province_definition_identifiers(),
	// 	GROUP,
	// 	COUNTRY,
	// 	PROVINCE,
	// 	PROVINCE_ID,
	// 	NO_IDENTIFIER
	// );
	for (ProvinceDefinition const& province : definition_manager.get_map_definition().get_province_definitions()) {
		ret &= add_condition(
			province.get_identifier(),
			_parse_condition_node_list_callback<PROVINCE>,
			_execute_condition_node_list_single_scope_callback<expect_true, require_all>(
				[&province](
					InstanceManager const& instance_manager, scope_t const& current_scope, scope_t const& this_scope,
					scope_t const& from_scope
				) -> scope_t {
					return &instance_manager.get_map_instance().get_province_instance_from_definition(province);
				}
			)
		);
	}

	/* Conditions from other registries */
	// import_identifiers(
	// 	definition_manager.get_politics_manager().get_ideology_manager().get_ideology_identifiers(),
	// 	REAL,
	// 	COUNTRY,
	// 	NO_SCOPE,
	// 	IDEOLOGY,
	// 	NO_IDENTIFIER
	// );

	// import_identifiers(
	// 	definition_manager.get_politics_manager().get_issue_manager().get_reform_group_identifiers(),
	// 	IDENTIFIER,
	// 	COUNTRY,
	// 	NO_SCOPE,
	// 	REFORM_GROUP,
	// 	REFORM
	// );

	// import_identifiers(
	// 	definition_manager.get_politics_manager().get_issue_manager().get_reform_identifiers(),
	// 	REAL,
	// 	COUNTRY,
	// 	NO_SCOPE,
	// 	REFORM,
	// 	NO_IDENTIFIER
	// );

	// import_identifiers(
	// 	definition_manager.get_politics_manager().get_issue_manager().get_issue_identifiers(),
	// 	REAL,
	// 	COUNTRY,
	// 	NO_SCOPE,
	// 	ISSUE,
	// 	NO_IDENTIFIER
	// );

	// import_identifiers(
	// 	definition_manager.get_pop_manager().get_pop_type_identifiers(),
	// 	REAL,
	// 	COUNTRY,
	// 	NO_SCOPE,
	// 	POP_TYPE,
	// 	NO_IDENTIFIER
	// );

	// import_identifiers(
	// 	definition_manager.get_research_manager().get_technology_manager().get_technology_identifiers(),
	// 	BOOLEAN_INT,
	// 	COUNTRY,
	// 	NO_SCOPE,
	// 	TECHNOLOGY,
	// 	NO_IDENTIFIER
	// );

	// import_identifiers(
	// 	definition_manager.get_economy_manager().get_good_definition_manager().get_good_definition_identifiers(),
	// 	INTEGER,
	// 	COUNTRY,
	// 	NO_SCOPE,
	// 	TRADE_GOOD,
	// 	NO_IDENTIFIER
	// );

	if (
		add_condition(
			"root condition",
			_parse_condition_node_list_callback<no_scope_change, all_scopes_allowed, top_scope>,
			_execute_condition_node_list_single_scope_callback<expect_true, require_all>(_change_scope_keep_current_scope)
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
