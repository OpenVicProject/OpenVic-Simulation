#include "Condition.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

using no_argument_t = ConditionNode::no_argument_t;
using this_argument_t = ConditionNode::this_argument_t;
using from_argument_t = ConditionNode::from_argument_t;
using special_argument_t = ConditionNode::special_argument_t;
using integer_t = ConditionNode::integer_t;
using argument_t = ConditionNode::argument_t;

using no_scope_t = ConditionNode::no_scope_t;
using scope_t = ConditionNode::scope_t;

static constexpr std::string_view THIS_KEYWORD = "THIS";
static constexpr std::string_view FROM_KEYWORD = "FROM";

// Used for this_culture_union
static constexpr const char this_union_keyword[] = "this_union";
// Used for controlled_by
static constexpr const char owner_keyword[] = "owner";

ConditionNode::ConditionNode(
	Condition const* new_condition,
	argument_t&& new_argument
) : condition { new_condition },
	argument { std::move(new_argument) } {}

bool ConditionNode::execute(
	InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope, scope_t from_scope
) const {
	if (condition == nullptr) {
		Logger::error("ConditionNode has no condition!");
		return false;
	}

	return condition->get_execute_callback()(
		*condition, instance_manager, current_scope, this_scope, from_scope, argument
	);
}

struct print_argument_visitor_t {
	std::ostream& stream;
	size_t indent = 0;
	static constexpr size_t indent_width = 2;
	Condition const* last_condition = nullptr;

	void print_newline_indent() {
		stream << "\n" << std::setw(indent * indent_width) << "";
	}
	template<typename P1, typename P2>
	void print_pair(
		std::string_view first_key, std::string_view second_key, std::pair<P1, P2> const& values
	) {
		stream << "{";
		indent++;
		print_newline_indent();
		stream << first_key << " = " << values.first;
		print_newline_indent();
		stream << second_key << " = " << values.second;
		indent--;
		print_newline_indent();
		stream << "}";
	}
	void operator()(no_argument_t arg) {
		stream << "<NO_ARGUMENT>";
	}
	void operator()(this_argument_t arg) {
		stream << THIS_KEYWORD;
	}
	void operator()(from_argument_t arg) {
		stream << FROM_KEYWORD;
	}
	void operator()(special_argument_t arg) {
		const std::string_view identifier =
			last_condition != nullptr ? last_condition->get_identifier() : std::string_view {};
		if (identifier == "this_culture_union") {
			stream << this_union_keyword;
		} else if (identifier == "controlled_by") {
			stream << owner_keyword;
		} else {
			stream << "<SPECIAL_ARGUMENT>";
		}
	}
	void operator()(std::vector<ConditionNode> const& arg) {
		stream << "{";

		if (!arg.empty()) {
			indent++;

			for (ConditionNode const& child : arg) {
				print_newline_indent();
				stream << child;
			}

			indent--;
			print_newline_indent();
		}

		stream << "}";
	}
	void operator()(bool arg) {
		stream << (arg ? "yes" : "no");
	}
	void operator()(std::pair<PopType const*, fixed_point_t> arg) {
		print_pair("type", "value", arg);
	}
	void operator()(std::pair<bool, bool> arg) {
		print_pair(
			"in_whole_capital_state", "limit_to_world_greatest_level",
			std::pair { arg.first ? "yes" : "no", arg.second ? "yes" : "no" }
		);
	}
	void operator()(std::pair<Ideology const*, fixed_point_t> arg) {
		print_pair("ideology", "value", arg);
	}
	void operator()(std::vector<PopType const*> const& arg) {
		stream << "{";

		if (!arg.empty()) {
			indent++;

			for (PopType const* pop_type : arg) {
				print_newline_indent();
				stream << "worker = " << pop_type;
			}

			indent--;
			print_newline_indent();
		}

		stream << "}";
	}
	void operator()(std::pair<CountryDefinition const*, fixed_point_t> arg) {
		print_pair("who", "value", arg);
	}
	void operator()(std::pair<this_argument_t, fixed_point_t> arg) {
		print_pair("who", "value", std::pair { THIS_KEYWORD, arg.second });
	}
	void operator()(std::pair<from_argument_t, fixed_point_t> arg) {
		print_pair("who", "value", std::pair { FROM_KEYWORD, arg.second });
	}
	void operator()(std::pair<std::string, fixed_point_t> const& arg) {
		print_pair("which", "value", arg);
	}
	void operator()(auto const& arg) {
		stream << arg;
	}
};

static std::ostream& print_argument(
	std::ostream& stream, argument_t const& argument, Condition const* starting_last_condition
) {
	std::visit(print_argument_visitor_t { stream }, argument);
	return stream;
}

// std::ostream& OpenVic::operator<<(std::ostream& stream, ConditionNode::argument_t const& argument) {
// 	return print_argument(stream, argument, nullptr);
// }

std::string OpenVic::argument_to_string(ConditionNode::argument_t const& argument) {
	std::ostringstream stream;
	print_argument(stream, argument, nullptr);
	return stream.str();
}

std::ostream& OpenVic::operator<<(std::ostream& stream, ConditionNode const& node) {
	stream << node.get_condition() << " = ";
	return print_argument(stream, node.get_argument(), node.get_condition());
}

Condition::Condition(
	std::string_view new_identifier,
	parse_callback_t&& new_parse_callback,
	execute_callback_t&& new_execute_callback,
	void const* new_condition_data
) : HasIdentifier { new_identifier },
	parse_callback { std::move(new_parse_callback) },
	execute_callback { std::move(new_execute_callback) },
	condition_data { new_condition_data } {}

bool ConditionManager::add_condition(
	std::string_view identifier,
	Condition::parse_callback_t&& parse_callback,
	Condition::execute_callback_t&& execute_callback,
	void const* condition_data
) {
	if (identifier.empty()) {
		Logger::error("Invalid condition identifier - empty!");
		return false;
	}

	if (parse_callback == nullptr) {
		Logger::error("Condition \"", identifier, "\" has no parse callback!");
		return false;
	}

	if (execute_callback == nullptr) {
		Logger::error("Condition \"", identifier, "\" has no execute callback!");
		return false;
	}

	return conditions.add_item({
		identifier,
		std::move(parse_callback),
		std::move(execute_callback),
		condition_data
	});
}

ConditionManager::ConditionManager(DefinitionManager const& new_definition_manager)
	: root_condition { nullptr }, definition_manager { new_definition_manager } {}

Callback<Condition const&, ast::NodeCPtr> auto ConditionManager::expect_condition_node(
	scope_type_t current_scope, scope_type_t this_scope,
	scope_type_t from_scope, Callback<ConditionNode&&> auto callback
) const {
	return [this, current_scope, this_scope, from_scope, callback](
		Condition const& condition, ast::NodeCPtr node
	) -> bool {
		return condition.get_parse_callback()(
			condition, definition_manager, current_scope, this_scope, from_scope, node,
			[callback, &condition](argument_t&& argument) mutable -> bool {
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
	scope_type_t current_scope, scope_type_t this_scope, scope_type_t from_scope,
	Callback<ConditionNode&&> auto callback, LengthCallback auto length_callback, bool top_scope
) const {
	return conditions.expect_item_dictionary_and_length_and_default(
		std::move(length_callback),
		top_scope ? top_scope_fallback : key_value_invalid_callback,
		expect_condition_node(current_scope, this_scope, from_scope, std::move(callback))
	);
}

NodeCallback auto ConditionManager::expect_condition_node_list(
	scope_type_t current_scope, scope_type_t this_scope, scope_type_t from_scope,
	Callback<ConditionNode&&> auto callback, bool top_scope
) const {
	return expect_condition_node_list_and_length(
		definition_manager, current_scope, this_scope, from_scope, std::move(callback), default_length_callback, top_scope
	);
}

node_callback_t ConditionManager::expect_condition_script(
	scope_type_t initial_scope, scope_type_t this_scope,
	scope_type_t from_scope, callback_t<ConditionNode&&> callback
) const {
	return [this, initial_scope, this_scope, from_scope, callback](ast::NodeCPtr node) mutable -> bool {
		if (root_condition != nullptr) {
			return expect_condition_node(
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

static bool _parse_condition_node_unimplemented(
	Condition const& condition, DefinitionManager const& definition_manager, scope_type_t current_scope,
	scope_type_t this_scope, scope_type_t from_scope, ast::NodeCPtr node, callback_t<argument_t&&> callback
) {
	Logger::error("Cannot parse condition \"", condition.get_identifier(), "\" - callback unimplemented!");
	return false;
}

// If CHANGE_SCOPE is NO_SCOPE then current_scope is propagated through, otherwise the scope changes to CHANGE_SCOPE
// or this_scope/from_scope if CHANGE_SCOPE is THIS or FROM
template<scope_type_t CHANGE_SCOPE, scope_type_t ALLOWED_SCOPES, bool TOP_SCOPE>
bool ConditionManager::_parse_condition_node_list_callback(
	Condition const& condition, DefinitionManager const& definition_manager, scope_type_t current_scope,
	scope_type_t this_scope, scope_type_t from_scope, ast::NodeCPtr node, callback_t<argument_t&&> callback
) {
	using enum scope_type_t;

	// if (!share_scope_type(current_scope, ALLOWED_SCOPES & ALL_SCOPES)) {
	// 	Logger::error(
	// 		"Error parsing condition \"", condition.get_identifier(),
	// 		"\": scope mismatch for condition node list - expected ", ALLOWED_SCOPES, ", got ", current_scope
	// 	);
	// 	return false;
	// }

	ConditionManager const& condition_manager = definition_manager.get_script_manager().get_condition_manager();

	std::vector<ConditionNode> children;

	const scope_type_t new_scope =
		CHANGE_SCOPE == NO_SCOPE ? current_scope :
		CHANGE_SCOPE == THIS ? this_scope :
		CHANGE_SCOPE == FROM ? from_scope :
		CHANGE_SCOPE;

	if (new_scope == NO_SCOPE) {
		Logger::error(
			"Error parsing condition \"", condition.get_identifier(),
			"\": invalid scope change for condition node list - went from ", current_scope, " to ", new_scope,
			" based on change scope ", CHANGE_SCOPE, " and this/from scope ", this_scope, "/", from_scope
		);
		return false;
	}

	bool ret = condition_manager.expect_condition_node_list_and_length(
		new_scope, this_scope, from_scope,
		vector_callback(children), reserve_length_callback(children), TOP_SCOPE
	)(node);

	ret &= callback(std::move(children));

	return ret;
}

/* Callback for conditions with a special case as well as a regular value type:
 *  - special_case_triggered(condition, definition_manager, callback, str, ret)
 *    - str is the condition's value parsed as an identifier or string
 *    - special_case_triggered is the bool return value which determines whether to return ret or continue with the regular
 *      value parsing */
using special_callback_t = bool (*)(
	Condition const&, DefinitionManager const&, callback_t<argument_t&&>, std::string_view, bool&
);

static bool _parse_condition_node_special_callback_bool(
	Condition const& condition, DefinitionManager const& definition_manager, callback_t<argument_t&&> callback,
	std::string_view str, bool& ret
) {
	if (StringUtils::strings_equal_case_insensitive(str, "yes")) {
		ret = callback(true);
		return true;
	}

	if (StringUtils::strings_equal_case_insensitive(str, "no")) {
		ret = callback(false);
		return true;
	}

	return false;
}

template<char const* KEYWORD>
static bool _parse_condition_node_special_callback_keyword(
	Condition const& condition, DefinitionManager const& definition_manager, callback_t<argument_t&&> callback,
	std::string_view str, bool& ret
) {
	if (StringUtils::strings_equal_case_insensitive(str, KEYWORD)) {
		ret = callback(special_argument_t {});
		return true;
	}

	return false;
}

static bool _parse_condition_node_special_callback_country(
	Condition const& condition, DefinitionManager const& definition_manager, callback_t<argument_t&&> callback,
	std::string_view str, bool& ret
) {
	CountryDefinition const* country =
		definition_manager.get_country_definition_manager().get_country_definition_by_identifier(str);

	if (country != nullptr) {
		ret = callback(country);
		return true;
	}

	return false;
}

static bool _parse_condition_node_special_callback_building_type_type(
	Condition const& condition, DefinitionManager const& definition_manager, callback_t<argument_t&&> callback,
	std::string_view str, bool& ret
) {
	if (definition_manager.get_economy_manager().get_building_type_manager().get_building_type_types().contains(str)) {
		ret = callback(std::string { str });
		return true;
	}

	return false;
}

template<typename T>
concept IsValidArgument =
	// Value arguments
	std::same_as<T, bool> || std::same_as<T, std::string> || std::same_as<T, integer_t> ||
	std::same_as<T, fixed_point_t> ||

	// Game object arguments
	std::same_as<T, CountryDefinition const*> || std::same_as<T, ProvinceDefinition const*> ||
	std::same_as<T, GoodDefinition const*> || std::same_as<T, Continent const*> || std::same_as<T, BuildingType const*> ||
	std::same_as<T, Issue const*> || std::same_as<T, WargoalType const*> || std::same_as<T, PopType const*> ||
	std::same_as<T, CultureGroup const*> || std::same_as<T, Culture const*> || std::same_as<T, Religion const*> ||
	std::same_as<T, GovernmentType const*> || std::same_as<T, Ideology const*> || std::same_as<T, Reform const*> ||
	std::same_as<T, NationalValue const*> || std::same_as<T, Invention const*> ||
	std::same_as<T, TechnologySchool const*> || std::same_as<T, Crime const*> || std::same_as<T, Region const*> ||
	std::same_as<T, TerrainType const*> || std::same_as<T, Strata const*> ||

	// Multi-value arguments
	std::same_as<T, std::pair<PopType const*, fixed_point_t>> || std::same_as<T, std::pair<bool, bool>> ||
	std::same_as<T, std::pair<Ideology const*, fixed_point_t>> || std::same_as<T, std::vector<PopType const*>> ||
	std::same_as<T, std::pair<std::string, fixed_point_t>>;

// ALLOWED_SCOPES is a bitfield indicating valid values of current_scope, as well as whether the value is allowed to be
// THIS or FROM corresponding to the special argument types this_argument_t and from_argument_t respectively.
template<
	IsValidArgument T, scope_type_t ALLOWED_SCOPES = scope_type_t::ALL_SCOPES, special_callback_t SPECIAL_CALLBACK = nullptr
>
static bool _parse_condition_node_value_callback(
	Condition const& condition, DefinitionManager const& definition_manager, scope_type_t current_scope,
	scope_type_t this_scope, scope_type_t from_scope, ast::NodeCPtr node, callback_t<argument_t&&> callback
) {
	using enum scope_type_t;

	// if (!share_scope_type(current_scope, ALLOWED_SCOPES & ALL_SCOPES)) {
	// 	Logger::error(
	// 		"Error parsing condition \"", condition.get_identifier(),
	// 		"\": scope mismatch for ", typeid(T).name(), " value - expected ", ALLOWED_SCOPES, ", got ", current_scope
	// 	);
	// 	return false;
	// }

	// All possible value types can also be interpreted as an identifier or string, so we shouldn't get any unwanted error
	// messages if the value is a regular value rather than THIS or FROM. In fact if expect_identifier_or_string returns false
	// when checking for THIS or FROM then we can be confident that it would also return false when parsing a regular value.

	if constexpr (share_scope_type(ALLOWED_SCOPES, THIS | FROM) || SPECIAL_CALLBACK != nullptr) {
		std::string_view str;

		if (!expect_identifier_or_string(assign_variable_callback(str))(node)) {
			Logger::error(
			"Error parsing condition \"", condition.get_identifier(),
			"\": failed to parse identifier or string when checking for THIS and/or FROM condition argument!"
		);
			return false;
		}

		if constexpr (share_scope_type(ALLOWED_SCOPES, THIS)) {
			if (StringUtils::strings_equal_case_insensitive(str, THIS_KEYWORD)) {
				return callback(this_argument_t {});
			}
		}

		if constexpr (share_scope_type(ALLOWED_SCOPES, FROM)) {
			if (StringUtils::strings_equal_case_insensitive(str, FROM_KEYWORD)) {
				return callback(from_argument_t {});
			}
		}

		if constexpr (SPECIAL_CALLBACK != nullptr) {
			bool ret = true;
			if ((*SPECIAL_CALLBACK)(condition, definition_manager, callback, str, ret)) {
				return ret;
			}
		}
	}

	bool ret = true;
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
	} else if constexpr (std::same_as<T, BuildingType const*>) {
		ret = definition_manager.get_economy_manager().get_building_type_manager().expect_building_type_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, Issue const*>) {
		ret = definition_manager.get_politics_manager().get_issue_manager().expect_issue_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, WargoalType const*>) {
		ret = definition_manager.get_military_manager().get_wargoal_type_manager().expect_wargoal_type_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, PopType const*>) {
		ret = definition_manager.get_pop_manager().expect_pop_type_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, CultureGroup const*>) {
		ret = definition_manager.get_pop_manager().get_culture_manager().expect_culture_group_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, Culture const*>) {
		ret = definition_manager.get_pop_manager().get_culture_manager().expect_culture_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, Religion const*>) {
		ret = definition_manager.get_pop_manager().get_religion_manager().expect_religion_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, GovernmentType const*>) {
		ret = definition_manager.get_politics_manager().get_government_type_manager()
			.expect_government_type_identifier_or_string(
				assign_variable_callback_pointer(value)
			)(node);
	} else if constexpr (std::same_as<T, Ideology const*>) {
		ret = definition_manager.get_politics_manager().get_ideology_manager().expect_ideology_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, Reform const*>) {
		ret = definition_manager.get_politics_manager().get_issue_manager().expect_reform_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, NationalValue const*>) {
		ret = definition_manager.get_politics_manager().get_national_value_manager().expect_national_value_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, Invention const*>) {
		ret = definition_manager.get_research_manager().get_invention_manager().expect_invention_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, TechnologySchool const*>) {
		ret = definition_manager.get_research_manager().get_technology_manager().expect_technology_school_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, Crime const*>) {
		ret = definition_manager.get_crime_manager().expect_crime_modifier_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, Region const*>) {
		ret = definition_manager.get_map_definition().expect_region_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, TerrainType const*>) {
		ret = definition_manager.get_map_definition().get_terrain_type_manager().expect_terrain_type_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, Strata const*>) {
		ret = definition_manager.get_pop_manager().expect_strata_identifier_or_string(
			assign_variable_callback_pointer(value)
		)(node);
	} else if constexpr (std::same_as<T, std::pair<PopType const*, fixed_point_t>>) {
		ret = expect_dictionary_keys(
			"type", ONE_EXACTLY, definition_manager.get_pop_manager().expect_pop_type_identifier_or_string(
				assign_variable_callback_pointer(value.first)
			),
			"value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(value.second))
		)(node);
	} else if constexpr (std::same_as<T, std::pair<bool, bool>>) {
		ret = expect_dictionary_keys(
			"in_whole_capital_state", ONE_EXACTLY, assign_variable_callback(value.first),
			"limit_to_world_greatest_level", ONE_EXACTLY, assign_variable_callback(value.second)
		)(node);
	} else if constexpr (std::same_as<T, std::pair<Ideology const*, fixed_point_t>>) {
		ret = expect_dictionary_keys(
			"ideology", ONE_EXACTLY,
				definition_manager.get_politics_manager().get_ideology_manager().expect_ideology_identifier_or_string(
					assign_variable_callback_pointer(value.first)
				),
			"value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(value.second))
		)(node);
	} else if constexpr (std::same_as<T, std::vector<PopType const*>>) {
		ret = expect_dictionary_keys(
			"worker", ONE_OR_MORE, definition_manager.get_pop_manager().expect_pop_type_identifier_or_string(
				vector_callback_pointer(value)
			)
		)(node);
	} else if constexpr (std::same_as<T, std::pair<std::string, fixed_point_t>>) {
		ret = expect_dictionary_keys(
			"which", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback_string(value.first)),
			"value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(value.second))
		)(node);
	} else {
		Logger::error("Cannot parse condition \"", condition.get_identifier(), "\": unknown value type!");
		ret = false;
	}

	if (ret) {
		ret &= callback(std::move(value));
	}

	return ret;
}

static bool _parse_condition_node_who_value_callback(
	Condition const& condition, DefinitionManager const& definition_manager, scope_type_t current_scope,
	scope_type_t this_scope, scope_type_t from_scope, ast::NodeCPtr node, callback_t<argument_t&&> callback
) {
	using enum scope_type_t;

	// if (!share_scope_type(current_scope, COUNTRY)) {
	// 	Logger::error(
	// 		"Error parsing condition \"", condition.get_identifier(),
	// 		"\": scope mismatch - expected ", COUNTRY, ", got ", current_scope
	// 	);
	// 	return false;
	// }

	std::string_view str;
	fixed_point_t value;

	if (
		expect_dictionary_keys(
			"who", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(str)),
			"value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(value))
		)(node)
	) {
		if (StringUtils::strings_equal_case_insensitive(str, THIS_KEYWORD)) {
			return callback(std::pair { this_argument_t {}, value });
		}

		if (StringUtils::strings_equal_case_insensitive(str, FROM_KEYWORD)) {
			return callback(std::pair { from_argument_t {}, value });
		}

		CountryDefinition const* country =
			definition_manager.get_country_definition_manager().get_country_definition_by_identifier(str);

		if (country != nullptr) {
			return callback(std::pair { country, value });
		}

		Logger::error(
			"Error parsing condition \"", condition.get_identifier(), "\": unknown country \"", str, "\""
		);
	}

	return false;
}

// EXECUTE CALLBACK HELPERS

static bool _execute_condition_node_unimplemented(
	Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
	scope_t from_scope, argument_t const& argument
) {
	Logger::error("Cannot execute condition \"", condition.get_identifier(), "\" - callback unimplemented!");
	return false;
}
static bool _execute_condition_node_unimplemented_no_error(
	Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
	scope_t from_scope, argument_t const& argument
) {
	return false;
}

/*
	- Convert to CountryInstance const*...
	 - CountryInstance const* = self
	 - State const* = state's owner
	 - ProvinceInstance const* = province owner
	 - Pop const* = location's owner

	- Convert to State const*...
	 - CountryInstance const* = any owned state
	 - State const* = self
	 - ProvinceInstance const* = province's state
	 - Pop const* = location's state

	- Convert to ProvinceInstance const*...
	 - CountryInstance const* = any owned province
	 - State const* = any province in state
	 - ProvinceInstance const* = self
	 - Pop const* = location

	- Convert to Pop const*...
	 - CountryInstance const* = any pop in country
	 - State const* = any pop in state
	 - ProvinceInstance const* = any pop in province
	 - Pop const* = self
*/

static constexpr bool expect_true = true;
static constexpr bool expect_false = false;
static constexpr bool require_all = true;
static constexpr bool require_any = false;

/* - EXPECTED_VALUE = what we want child nodes to evaluate to, e.g. true for AND and OR, false for NOT
 * - REQUIRE_ALL = whether all children must evaluate to expected_value or only one, e.g. true for AND and NOT, false for OR */
template<bool EXPECTED_VALUE, bool REQUIRE_ALL, typename Container>
static constexpr bool _execute_iterative(
	Container const& items, Callback<typename Container::value_type const&> auto item_callback
) {
	for (typename Container::value_type const& item : items) {
		// check what happens here when value_type is a non-const pointer?
		if (item_callback(item) == (EXPECTED_VALUE != REQUIRE_ALL)) {
			return !REQUIRE_ALL;
		}
	}

	return REQUIRE_ALL;
}

template<
	typename T,
	Callback<
		// bool(condition, instance_manager, current_scope)
		Condition const&, InstanceManager const&, T const*
	> CallbackFunc
>
struct execute_condition_node_visitor_t;

#define EXECUTE_CONDITION_NODE_VISITOR(T) \
	template< \
		Callback< \
			/* bool(condition, instance_manager, current_scope) */ \
			Condition const&, InstanceManager const&, T const* \
		> CallbackFunc \
	> \
	struct execute_condition_node_visitor_t<T, CallbackFunc> { \
		CallbackFunc const& callback; \
		Condition const& condition; \
		InstanceManager const& instance_manager; \
		constexpr bool operator()(no_scope_t no_scope) const { \
			Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!"); \
			return false; \
		}

EXECUTE_CONDITION_NODE_VISITOR(CountryInstance)

	constexpr bool operator()(CountryInstance const* country) const {
		return callback(condition, instance_manager, country);
	}
	constexpr bool operator()(State const* state) const {
		CountryInstance const* country = state->get_owner();
		return country != nullptr && (*this)(country);
	}
	constexpr bool operator()(ProvinceInstance const* province) const {
		CountryInstance const* country = province->get_owner();
		return country != nullptr && (*this)(country);
	}
	constexpr bool operator()(Pop const* pop) const {
		ProvinceInstance const* province = pop->get_location();
		return province != nullptr && (*this)(province);
	}
};

EXECUTE_CONDITION_NODE_VISITOR(State)

	bool operator()(CountryInstance const* country) const {
		return _execute_iterative<expect_true, require_any>(country->get_states(), *this);
	}
	constexpr bool operator()(State const* state) const {
		return callback(condition, instance_manager, state);
	}
	constexpr bool operator()(ProvinceInstance const* province) const {
		State const* state = province->get_state();
		return state != nullptr && (*this)(state);
	}
	constexpr bool operator()(Pop const* pop) const {
		ProvinceInstance const* province = pop->get_location();
		return province != nullptr && (*this)(province);
	}
};

EXECUTE_CONDITION_NODE_VISITOR(ProvinceInstance)

	bool operator()(CountryInstance const* country) const {
		return _execute_iterative<expect_true, require_any>(country->get_owned_provinces(), *this);
	}
	bool operator()(State const* state) const {
		return _execute_iterative<expect_true, require_any>(state->get_provinces(), *this);
	}
	constexpr bool operator()(ProvinceInstance const* province) const {
		return callback(condition, instance_manager, province);
	}
	constexpr bool operator()(Pop const* pop) const {
		ProvinceInstance const* province = pop->get_location();
		return province != nullptr && (*this)(province);
	}
};

EXECUTE_CONDITION_NODE_VISITOR(Pop)

	bool operator()(CountryInstance const* country) const {
		return _execute_iterative<expect_true, require_any>(country->get_owned_provinces(), *this);
	}
	bool operator()(State const* state) const {
		return _execute_iterative<expect_true, require_any>(state->get_provinces(), *this);
	}
	bool operator()(ProvinceInstance const* province) const {
		return _execute_iterative<expect_true, require_any>(province->get_pops(), *this);
	}
	constexpr bool operator()(Pop const& pop) const {
		return (*this)(&pop);
	}
	constexpr bool operator()(Pop const* pop) const {
		return callback(condition, instance_manager, pop);
	}
};

template<typename T, typename... Args>
static constexpr auto _execute_condition_node_convert_scope(
	Callback<
		// bool(condition, instance_manager, current_scope, args...)
		Condition const&, InstanceManager const&, T const*, Args...
	> auto callback
) {
	return [callback](
		Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
		Args... args
	) -> bool {
		const auto visitor =
			[callback, &args...](
				Condition const& condition, InstanceManager const& instance_manager, T const* current_scope
			) -> bool {
				return callback(condition, instance_manager, current_scope, args...);
			};
		return std::visit(execute_condition_node_visitor_t<T, decltype(visitor)> {
			visitor, condition, instance_manager
		}, current_scope);
	};
}

// template<typename RETURN_TYPE, typename SCOPE_TYPE, typename... Args>
// static constexpr auto _execute_condition_node_try_cast_scope(
// 	Functor<
// 		// return_type(condition, instance_manager, cast_scope, args...)
// 		RETURN_TYPE, Condition const&, InstanceManager const&, SCOPE_TYPE, Args...
// 	> auto success_callback,
// 	Functor<
// 		// return_type(condition, instance_manager, first_scope, args...)
// 		RETURN_TYPE, Condition const&, InstanceManager const&, scope_t, Args...
// 	> auto failure_callback
// ) {
// 	return [success_callback, failure_callback](
// 		Condition const& condition, InstanceManager const& instance_manager, scope_t first_scope, Args... args
// 	) -> RETURN_TYPE {
// 		SCOPE_TYPE const* cast_scope = std::get_if<SCOPE_TYPE>(&first_scope);

// 		if (cast_scope != nullptr) {
// 			return success_callback(condition, instance_manager, *cast_scope, args...);
// 		} else {
// 			return failure_callback(condition, instance_manager, first_scope, args...);
// 		}
// 	};
// }

// template<typename RETURN_TYPE, typename SCOPE_TYPE, typename... Args>
// static constexpr auto _execute_condition_node_cast_scope(
// 	Functor<
// 		// return_type(condition, instance_manager, first_scope, args...)
// 		RETURN_TYPE, Condition const&, InstanceManager const&, SCOPE_TYPE, Args...
// 	> auto callback
// ) {
// 	return _execute_condition_node_try_cast_scope<RETURN_TYPE, SCOPE_TYPE, Args...>(
// 		std::move(callback),
// 		[](
// 			Condition const& condition, InstanceManager const& instance_manager, scope_t first_scope, Args... args
// 		) -> RETURN_TYPE {
// 			Logger::error(
// 				"Error executing condition \"", condition.get_identifier(), "\": invalid scope for condition node - expected ",
// 				typeid(SCOPE_TYPE).name()
// 			);
// 			return {};
// 		}
// 	);
// }

// template<typename RETURN_TYPE, typename FIRST_SCOPE_TYPE, typename SECOND_SCOPE_TYPE, typename... Args>
// static constexpr auto _execute_condition_node_cast_two_scopes(
// 	Functor<
// 		// return_type(condition, instance_manager, first_scope, second_scope, args...)
// 		RETURN_TYPE, Condition const&, InstanceManager const&, FIRST_SCOPE_TYPE, SECOND_SCOPE_TYPE, Args...
// 	> auto callback
// ) {
// 	return _execute_condition_node_cast_scope<RETURN_TYPE, FIRST_SCOPE_TYPE, scope_t, Args...>(
// 		[callback](
// 			Condition const& condition, InstanceManager const& instance_manager, FIRST_SCOPE_TYPE cast_first_scope,
// 			scope_t second_scope, Args... args
// 		) -> RETURN_TYPE {
// 			return _execute_condition_node_cast_scope<RETURN_TYPE, SECOND_SCOPE_TYPE, FIRST_SCOPE_TYPE, Args...>(
// 				[callback](
// 					Condition const& condition, InstanceManager const& instance_manager,
// 					SECOND_SCOPE_TYPE cast_second_scope, FIRST_SCOPE_TYPE cast_first_scope, Args... args
// 				) -> RETURN_TYPE {
// 					return callback(condition, instance_manager, cast_first_scope, cast_second_scope, args...);
// 				}
// 			)(condition, instance_manager, second_scope, cast_first_scope, args...);
// 		}
// 	);
// }

// template<typename ARGUMENT_TYPE, typename SCOPE_TYPE, typename... OTHER_SCOPES>
// static constexpr auto _execute_condition_node_try_cast_scope_types(
// 	Callback<
// 		// bool(instance_manager, current_scope, this_scope, from_scope, argument)
// 		InstanceManager const&, SCOPE_TYPE, scope_t, scope_t, ARGUMENT_TYPE const&
// 	> auto callback, auto other_callbacks...
// ) {
// 	return [callback, other_callbacks](
// 		InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope, scope_t from_scope,
// 		ARGUMENT_TYPE const& argument
// 	) -> bool {
// 		return _execute_condition_node_try_cast_scope<bool, SCOPE_TYPE, scope_t, scope_t, ARGUMENT_TYPE const&>(
// 			callback,
// 			[other_callbacks](
// 				InstanceManager const& instance_manager, scope_t first_scope, scope_t this_scope, scope_t from_scope,
// 				ARGUMENT_TYPE const& argument
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
		// bool(condition, instance_manager, args..., argument)
		Condition const&, InstanceManager const&, Args..., T const&
	> auto success_callback,
	Callback<
		// bool(condition, instance_manager, args...,)
		Condition const&, InstanceManager const&, Args..., argument_t const&
	> auto failure_callback
) {
	return [success_callback, failure_callback](
		Condition const& condition, InstanceManager const& instance_manager, Args... args, argument_t const& argument
	) -> bool {
		if constexpr (std::same_as<T, CountryInstance const*>) {
			CountryDefinition const* const* value = std::get_if<CountryDefinition const*>(&argument);

			if (value != nullptr) {
				CountryInstance const* instance =
					&instance_manager.get_country_instance_manager().get_country_instance_from_definition(**value);

				return success_callback(condition, instance_manager, args..., instance);
			}
		} else if constexpr (std::same_as<T, ProvinceInstance const*>) {
			ProvinceDefinition const* const* value = std::get_if<ProvinceDefinition const*>(&argument);

			if (value != nullptr) {
				ProvinceInstance const* instance =
					&instance_manager.get_map_instance().get_province_instance_from_definition(**value);

				return success_callback(condition, instance_manager, args..., instance);
			}
		} else {
			T const* value = std::get_if<T>(&argument);

			if (value != nullptr) {
				return success_callback(condition, instance_manager, args..., *value);
			}
		}

		return failure_callback(condition, instance_manager, args..., argument);
	};
}

template<typename T, typename... Args>
static constexpr auto _execute_condition_node_cast_argument_callback(
	Callback<
		// bool(condition, instance_manager, args..., argument)
		Condition const&, InstanceManager const&, Args..., T const&
	> auto callback
) {
	return _execute_condition_node_try_cast_argument_callback<T, Args...>(
		std::move(callback),
		[](
			Condition const& condition, InstanceManager const& instance_manager, Args... args, argument_t const& argument
		) -> bool {
			Logger::error(
				"Error executing condition \"", condition.get_identifier(), "\", invalid argument: ",
				argument_to_string(argument)
			);
			// TODO - see comment below about ensuring consistent negation behaviour
			return false;
		}
	);
}

template<typename T>
static constexpr auto _execute_condition_node_value_or_this_or_from_callback(
	Callback<
		// bool(condition, instance_manager, current_scope, value)
		Condition const&, InstanceManager const&, scope_t, T const&
	> auto value_callback,
	Callback<
		// bool(condition, instance_manager, current_scope, this_scope)
		Condition const&, InstanceManager const&, scope_t, scope_t
	> auto this_callback,
	Callback<
		// bool(condition, instance_manager, current_scope, from_scope)
		Condition const&, InstanceManager const&, scope_t, scope_t
	> auto from_callback
) {
	return _execute_condition_node_try_cast_argument_callback<T, scope_t, scope_t, scope_t>(
		[value_callback](
			Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
			scope_t from_scope, T const& value
		) -> bool {
			return value_callback(condition, instance_manager, current_scope, value);
		},
		[this_callback, from_callback](
			Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
			scope_t from_scope, argument_t const& argument
		) -> bool {
			if (ConditionNode::is_this_argument(argument)) {
				return this_callback(condition, instance_manager, current_scope, this_scope);
			}

			if (ConditionNode::is_from_argument(argument)) {
				return from_callback(condition, instance_manager, current_scope, this_scope);
			}

			Logger::error(
				"Error executing condition \"", condition.get_identifier(), "\", invalid argument: ",
				argument_to_string(argument)
			);
			// TODO - see comment below about ensuring consistent negation behaviour
			return false;
		}
	);
}

template<typename T>
static constexpr auto _execute_condition_node_value_or_this_or_from_callback(
	Callback<
		// bool(condition, instance_manager, current_scope, value)
		Condition const&, InstanceManager const&, scope_t, T const&
	> auto value_callback,
	Callback<
		// bool(condition, instance_manager, current_scope, this_or_from_scope)
		Condition const&, InstanceManager const&, scope_t, scope_t
	> auto this_or_from_callback
) {
	return _execute_condition_node_value_or_this_or_from_callback<T>(
		std::move(value_callback), this_or_from_callback, this_or_from_callback
	);
}

// Handles THIS or FROM cases by changing them to values using this_or_from_to_value_callback and then using value_callback
template<typename Value>
static constexpr auto _execute_condition_node_value_or_change_this_or_from_callback(
	Callback<
		// bool(condition, instance_manager, current_scope, value)
		Condition const&, InstanceManager const&, scope_t, Value const&
	> auto value_callback,
	Functor<
		// value(condition, instance_manager, this_or_from_scope)
		Value, Condition const&, InstanceManager const&, scope_t
	> auto this_or_from_to_value_callback
) {
	return _execute_condition_node_value_or_this_or_from_callback<Value>(
		value_callback,
		[value_callback, this_or_from_to_value_callback](
			Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
			scope_t this_or_from_scope
		) -> bool {
			return value_callback(condition, instance_manager, current_scope, this_or_from_to_value_callback(
				condition, instance_manager, this_or_from_scope
			));
		}
	);
}

// Handles THIS or FROM cases by changing them to values using this_or_from_to_value_callback (after converting scope with
// _execute_condition_node_convert_scope) and then using value_callback
template<typename Value, typename Convert>
static constexpr auto _execute_condition_node_value_or_change_this_or_from_callback_convert(
	Callback<
		// bool(condition, instance_manager, current_scope, value)
		Condition const&, InstanceManager const&, scope_t, Value const&
	> auto value_callback,
	Functor<
		// value(condition, instance_manager, this_or_from_scope)
		Value, Condition const&, InstanceManager const&, Convert const*
	> auto this_or_from_to_value_callback
) {
	return _execute_condition_node_value_or_this_or_from_callback<Value>(
		value_callback,
		[value_callback, this_or_from_to_value_callback](
			Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
			scope_t this_or_from_scope
		) -> bool {
			return _execute_condition_node_convert_scope<Convert, scope_t>(
				[value_callback, this_or_from_to_value_callback](
					Condition const& condition, InstanceManager const& instance_manager, Convert const* this_or_from_scope,
					scope_t current_scope
				) -> bool {
					return value_callback(condition, instance_manager, current_scope, this_or_from_to_value_callback(
						condition, instance_manager, this_or_from_scope
					));
				}
			)(condition, instance_manager, this_or_from_scope, current_scope);
		}
	);
}

// template<typename T>
// static constexpr auto _execute_condition_node_value_or_cast_this_or_from_callback(
// 	Callback<
// 		// bool(condition, instance_manager, current_scope, value)
// 		Condition const&, InstanceManager const&, scope_t, T const&
// 	> auto callback
// ) {
// 	// If IS_THIS is false then the scope is FROM
// 	const auto cast_scope_callback = [callback]<bool IS_THIS>(
// 		Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_or_from_scope
// 	) -> bool {
// 		T const* cast_this_or_from_scope = std::get_if<T>(&this_or_from_scope);

// 		if (cast_this_or_from_scope == nullptr) {
// 			Logger::error(
// 				"Error executing condition \"", condition.get_identifier(), "\": invalid ", IS_THIS ? "THIS" : "FROM",
// 				" scope for condition node - expected ", typeid(T).name()
// 			);
// 			// TODO - are we sure the fail case is always false here? We may want to manipulate this elsewhere in the callchain
// 			// to ensure negated conditions always return the correct result.
// 			return false;
// 		}

// 		return callback(condition, instance_manager, current_scope, *cast_this_or_from_scope);
// 	};

// 	return _execute_condition_node_value_or_this_or_from_callback<T>(
// 		callback,
// 		[cast_scope_callback](
// 			Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
// 			scope_t this_or_from_scope
// 		) -> bool {
// 			return cast_scope_callback.template operator()<true>(
// 				condition, instance_manager, current_scope, this_or_from_scope
// 			);
// 		},
// 		[cast_scope_callback](
// 			Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
// 			scope_t this_or_from_scope
// 		) -> bool {
// 			return cast_scope_callback.template operator()<false>(
// 				condition, instance_manager, current_scope, this_or_from_scope
// 			);
// 		}
// 	);
// }

template<typename T>
static constexpr auto _execute_condition_node_value_or_convert_this_or_from_callback(
	Callback<
		// bool(condition, instance_manager, current_scope, value)
		Condition const&, InstanceManager const&, scope_t, T const*
	> auto callback
) {
	return _execute_condition_node_value_or_this_or_from_callback<T const*>(
		callback,
		[callback](
			Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
			scope_t this_or_from_scope
		) -> bool {
			return _execute_condition_node_convert_scope<T, scope_t>(
				[callback](
					Condition const& condition, InstanceManager const& instance_manager, T const* this_or_from_scope,
					scope_t current_scope
				) -> bool {
					return callback(condition, instance_manager, current_scope, this_or_from_scope);
				}
			)(condition, instance_manager, this_or_from_scope, current_scope);
		}
	);
}

static constexpr scope_t _change_scope_keep_current_scope(
	Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
	scope_t from_scope
) {
	return current_scope;
}

template<bool EXPECTED_VALUE, bool REQUIRE_ALL>
static constexpr bool _execute_condition_node_list(
	InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope, scope_t from_scope,
	std::vector<ConditionNode> const& condition_nodes
) {
	return _execute_iterative<EXPECTED_VALUE, REQUIRE_ALL>(
		condition_nodes,
		[&instance_manager, &current_scope, &this_scope, &from_scope](
			ConditionNode const& condition_node
		) -> bool {
			return condition_node.execute(instance_manager, current_scope, this_scope, from_scope);
		}
	);
}

/* - change_scope = returns the current scope for the child conditions to be executed with */
template<bool EXPECTED_VALUE, bool REQUIRE_ALL, typename CURRENT_SCOPE = scope_t>
static constexpr auto _execute_condition_node_list_single_scope_callback(
	Functor<
		// new_scope(condition, instance_manager, current_scope, this_scope, from_scope)
		scope_t, Condition const&, InstanceManager const&, CURRENT_SCOPE, scope_t, scope_t
	> auto change_scope
) {
	return [change_scope](
		Condition const& condition, InstanceManager const& instance_manager, CURRENT_SCOPE current_scope,
		scope_t this_scope, scope_t from_scope, argument_t const& argument
	) -> bool {
		return _execute_condition_node_cast_argument_callback<
			std::vector<ConditionNode>, CURRENT_SCOPE, scope_t, scope_t
		>(
			[change_scope](
				Condition const& condition, InstanceManager const& instance_manager, CURRENT_SCOPE current_scope,
				scope_t this_scope, scope_t from_scope, std::vector<ConditionNode> const& argument
			) -> bool {
				const scope_t new_scope = change_scope(condition, instance_manager, current_scope, this_scope, from_scope);

				if (ConditionNode::is_no_scope(new_scope)) {
					Logger::error(
						"Error executing condition \"", condition.get_identifier(),
						"\": invalid scope change for condition node list - no scope!"
					);
					// TODO - should this take EXPECTED_VALUE and REQUIRE_ALL into account?
					return false;
				}

				return _execute_condition_node_list<EXPECTED_VALUE, REQUIRE_ALL>(
					instance_manager, new_scope, this_scope, from_scope, argument
				);
			}
		)(condition, instance_manager, current_scope, this_scope, from_scope, argument);
	};
}

/* - change_scopes = returns the vector of current scopes for the child conditions to be executed with
 * - here EXPECTED_VALUE and REQUIRE_ALL refer to whether the results per scope are expected to be true and if all are needed,
 *   the conditions themselves are all expected to be true and are all required for each scope individually. */
template<bool EXPECTED_VALUE, bool REQUIRE_ALL, typename CONTAINER, typename CURRENT_SCOPE = scope_t>
static constexpr auto _execute_condition_node_list_multi_scope_callback(
	Functor<
		// new_scopes(condition, instance_manager, current_scope, this_scope, from_scope)
		CONTAINER, Condition const&, InstanceManager const&, CURRENT_SCOPE, scope_t, scope_t
	> auto change_scopes
) {
	return [change_scopes](
		Condition const& condition, InstanceManager const& instance_manager, CURRENT_SCOPE current_scope,
		scope_t this_scope, scope_t from_scope, argument_t const& argument
	) -> bool {
		return _execute_condition_node_cast_argument_callback<
			std::vector<ConditionNode>, CURRENT_SCOPE, scope_t, scope_t
		>(
			[change_scopes](
				Condition const& condition, InstanceManager const& instance_manager, CURRENT_SCOPE current_scope,
				scope_t this_scope, scope_t from_scope, std::vector<ConditionNode> const& argument
			) -> bool {
				return _execute_iterative<EXPECTED_VALUE, REQUIRE_ALL>(
					change_scopes(condition, instance_manager, current_scope, this_scope, from_scope),
					[&instance_manager, &this_scope, &from_scope, &argument](auto const& new_scope) -> bool {
						scope_t new_scope_final;

						if constexpr (std::same_as<decltype(new_scope), CountryDefinition const* const&>) {
							new_scope_final = &instance_manager.get_country_instance_manager()
								.get_country_instance_from_definition(*new_scope);
						} else if constexpr (std::same_as<decltype(new_scope), ProvinceDefinition const* const&>) {
							new_scope_final =
								&instance_manager.get_map_instance().get_province_instance_from_definition(*new_scope);
						} else if constexpr (
							// TODO - support CountryDefinition const& and ProvinceDefinition const&?
							std::same_as<decltype(new_scope), CountryInstance const&> ||
							std::same_as<decltype(new_scope), State const&> ||
							std::same_as<decltype(new_scope), ProvinceInstance const&> ||
							std::same_as<decltype(new_scope), Pop const&>
						) {
							new_scope_final = &new_scope;
						} else {
							new_scope_final = new_scope;
						}

						return _execute_condition_node_list<true, true>(
							instance_manager, new_scope_final, this_scope, from_scope, argument
						);
					}
				);
			}
		)(condition, instance_manager, current_scope, this_scope, from_scope, argument);
	};
}

bool ConditionManager::setup_conditions() {
	if (root_condition != nullptr || !conditions_empty()) {
		Logger::error("Cannot set up conditions - root condition is not null and/or condition registry is not empty!");
		return false;
	}

	using enum scope_type_t;

	static constexpr scope_type_t no_scope_change = NO_SCOPE;
	static constexpr scope_type_t all_scopes_allowed = ALL_SCOPES;
	static constexpr bool top_scope = true;

	bool ret = true;

	/* Special Scopes */
	ret &= add_condition(
		THIS_KEYWORD,
		_parse_condition_node_list_callback<THIS>,
		_execute_condition_node_list_single_scope_callback<expect_true, require_all>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope
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
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope
			) -> scope_t {
				return from_scope;
			}
		)
	);
	// Only from RebelType demands_enforced_trigger, changes scope to the country getting independence
	ret &= add_condition(
		"independence",
		_parse_condition_node_list_callback<COUNTRY>,
		_execute_condition_node_unimplemented
	);

	/* Trigger Country Scopes */
	ret &= add_condition(
		"all_core", // All core provinces of a country?
		_parse_condition_node_list_callback<PROVINCE, COUNTRY>,
		_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_multi_scope_callback<
				expect_true, require_all, ordered_set<ProvinceInstance*> const&, CountryInstance const*
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> ordered_set<ProvinceInstance*> const& {
					return current_scope->get_core_provinces();
				}
			)
		)
	);
	ret &= add_condition(
		"any_core", // Any country with a core in this province?
		_parse_condition_node_list_callback<COUNTRY, PROVINCE>,
		_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_multi_scope_callback<
				expect_true, require_any, ordered_set<CountryInstance*> const&, ProvinceInstance const*
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> ordered_set<CountryInstance*> const& {
					return current_scope->get_cores();
				}
			)
		)
	);
	ret &= add_condition(
		"any_greater_power", // Great powers, doesn't include secondary powers
		_parse_condition_node_list_callback<COUNTRY>,
		_execute_condition_node_list_multi_scope_callback<expect_true, require_any, std::vector<CountryInstance*> const&>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope
			) -> std::vector<CountryInstance*> const& {
				return instance_manager.get_country_instance_manager().get_great_powers();
			}
		)
	);
	ret &= add_condition(
		"any_neighbor_country",
		_parse_condition_node_list_callback<COUNTRY, COUNTRY>,
		_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_multi_scope_callback<
				expect_true, require_any, ordered_set<CountryInstance*> const&, CountryInstance const*
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> ordered_set<CountryInstance*> const& {
					return current_scope->get_neighbouring_countries();
				}
			)
		)
	);
	ret &= add_condition(
		"any_owned_province",
		_parse_condition_node_list_callback<PROVINCE, COUNTRY | PROVINCE>,
		_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_multi_scope_callback<
				expect_true, require_any, ordered_set<ProvinceInstance*> const&, CountryInstance const*
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> ordered_set<ProvinceInstance*> const& {
					return current_scope->get_owned_provinces();
				}
			)
		)
	);
	ret &= add_condition(
		"any_pop", // In vanilla this is only used at province scope
		_parse_condition_node_list_callback<POP, PROVINCE>,
		_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_multi_scope_callback<
				expect_true, require_any, plf::colony<Pop> const&, ProvinceInstance const*
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> plf::colony<Pop> const& {
					return current_scope->get_pops();
				}
			)
		)
	);
	ret &= add_condition(
		"any_sphere_member",
		_parse_condition_node_list_callback<COUNTRY, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"any_state",
		_parse_condition_node_list_callback<STATE, COUNTRY>,
		_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_multi_scope_callback<
				expect_true, require_any, ordered_set<State*> const&, CountryInstance const*
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> ordered_set<State*> const& {
					return current_scope->get_states();
				}
			)
		)
	);
	ret &= add_condition(
		"any_substate",
		_parse_condition_node_list_callback<COUNTRY, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"capital_scope",
		_parse_condition_node_list_callback<PROVINCE, COUNTRY>,
		_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_single_scope_callback<expect_true, require_all, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> scope_t {
					ProvinceInstance const* capital = current_scope->get_capital();

					if (capital == nullptr) {
						Logger::error(
							"Error executing condition \"", condition.get_identifier(),
							"\": cannot create province scope for capital_scope condition - country \"",
							current_scope->get_identifier(), "\" has no capital!"
						);
						return no_scope_t {};
					}

					return capital;
				}
			)
		)
	);
	ret &= add_condition(
		"country",
		_parse_condition_node_list_callback<COUNTRY, COUNTRY | POP>,
		_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_single_scope_callback<expect_true, require_all, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> scope_t {
					return current_scope;
				}
			)
		)
	);
	ret &= add_condition(
		"cultural_union",
		_parse_condition_node_list_callback<COUNTRY, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"overlord",
		_parse_condition_node_list_callback<COUNTRY, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"sphere_owner",
		_parse_condition_node_list_callback<COUNTRY, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"war_countries",
		_parse_condition_node_list_callback<COUNTRY, COUNTRY>,
		_execute_condition_node_unimplemented
	);

	/* Trigger State Scopes */
	ret &= add_condition(
		"any_neighbor_province",
		_parse_condition_node_list_callback<PROVINCE, PROVINCE>,
		_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_multi_scope_callback<
				expect_true, require_any, std::vector<ProvinceInstance const*> const&, ProvinceInstance const*
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> std::vector<ProvinceInstance const*> const& {
					return current_scope->get_adjacent_nonempty_land_provinces();
				}
			)
		)
	);

	/* Trigger Province Scopes */
	ret &= add_condition(
		"controller",
		_parse_condition_node_list_callback<COUNTRY, PROVINCE>,
		_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_single_scope_callback<expect_true, require_all, ProvinceInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> scope_t {
					CountryInstance const* controller = current_scope->get_controller();

					if (controller == nullptr) {
						Logger::error(
							"Error executing condition \"", condition.get_identifier(),
							"\": cannot create country scope for controller condition - province \"",
							current_scope->get_identifier(), "\" has no controller!"
						);
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
		_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_single_scope_callback<expect_true, require_all, ProvinceInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> scope_t {
					CountryInstance const* owner = current_scope->get_owner();

					if (owner == nullptr) {
						Logger::error(
							"Error executing condition \"", condition.get_identifier(),
							"\": cannot create country scope for owner condition - province \"",
							current_scope->get_identifier(), "\"has no owner!"
						);
						return no_scope_t {};
					}

					return owner;
				}
			)
		)
	);
	ret &= add_condition(
		"state_scope",
		_parse_condition_node_list_callback<STATE, PROVINCE>,
		_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_single_scope_callback<expect_true, require_all, ProvinceInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> scope_t {
					State const* state = current_scope->get_state();

					if (state == nullptr) {
						Logger::error(
							"Error executing condition \"", condition.get_identifier(),
							"\": cannot create state scope for state condition - province \"",
							current_scope->get_identifier(), "\"has no state!"
						);
						return no_scope_t {};
					}

					return state;
				}
			)
		)
	);

	/* Trigger Pop Scopes */
	ret &= add_condition(
		"location",
		_parse_condition_node_list_callback<PROVINCE, POP>,
		_execute_condition_node_convert_scope<Pop, scope_t, scope_t, argument_t const&>(
			_execute_condition_node_list_single_scope_callback<expect_true, require_all, Pop const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, Pop const* current_scope,
					scope_t this_scope, scope_t from_scope
				) -> scope_t {
					ProvinceInstance const* location = current_scope->get_location();

					if (location == nullptr) {
						Logger::error(
							"Error executing condition \"", condition.get_identifier(),
							"\": cannot create province scope for location condition - pop has no location!"
						);
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
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, integer_t argument
			) -> bool {
				return instance_manager.get_today().get_year() >= argument;
			}
		)
	);
	ret &= add_condition(
		"month",
		_parse_condition_node_value_callback<integer_t>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, integer_t argument
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
		_execute_condition_node_cast_argument_callback<std::string, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, std::string const& argument
			) -> bool {
				return instance_manager.get_global_flags().has_flag(argument);
			}
		)
	);
	ret &= add_condition(
		"is_canal_enabled",
		_parse_condition_node_value_callback<integer_t>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, integer_t argument
			) -> bool {
				return argument >= std::numeric_limits<MapInstance::canal_index_t>::min() &&
					argument <= std::numeric_limits<MapInstance::canal_index_t>::max() &&
					instance_manager.get_map_instance().is_canal_enabled(argument);
			}
		)
	);
	ret &= add_condition(
		"always",
		_parse_condition_node_value_callback<bool>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, bool argument
			) -> bool {
				return argument;
			}
		)
	);
	ret &= add_condition(
		"world_wars_enabled",
		_parse_condition_node_value_callback<bool>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, bool argument
			) -> bool {
				return instance_manager.get_politics_instance_manager().get_world_wars_enabled() == argument;
			}
		)
	);

	/* Country Scope Conditions */
	ret &= add_condition(
		"administration_spending",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_administration_spending_slider_value().get_value() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"ai",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->is_ai() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"alliance_with",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					return instance_manager.get_country_relation_manager().get_country_alliance(current_scope, value);
				}
			)
		)
	);
	ret &= add_condition(
		"average_consciousness",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY | PROVINCE>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, fixed_point_t argument
			) -> bool {
				struct visitor_t {

					Condition const& condition;
					fixed_point_t const& argument;

					bool operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
						return false;
					}

					constexpr bool operator()(CountryInstance const* country) const {
						return country->get_national_consciousness() >= argument;
					}
					constexpr bool operator()(State const* state) const {
						return state->get_average_consciousness() >= argument;
					}
					constexpr bool operator()(ProvinceInstance const* province) const {
						return province->get_average_consciousness() >= argument;
					}
					constexpr bool operator()(Pop const* pop) const {
						return pop->get_consciousness() >= argument;
					}
				};

				return std::visit(visitor_t { condition, argument }, current_scope);
			}
		)
	);
	ret &= add_condition(
		"average_militancy",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY | PROVINCE>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, fixed_point_t argument
			) -> bool {
				struct visitor_t {

					Condition const& condition;
					fixed_point_t const& argument;

					bool operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
						return false;
					}

					constexpr bool operator()(CountryInstance const* country) const {
						return country->get_national_militancy() >= argument;
					}
					constexpr bool operator()(State const* state) const {
						return state->get_average_militancy() >= argument;
					}
					constexpr bool operator()(ProvinceInstance const* province) const {
						return province->get_average_militancy() >= argument;
					}
					constexpr bool operator()(Pop const* pop) const {
						return pop->get_militancy() >= argument;
					}
				};

				return std::visit(visitor_t { condition, argument }, current_scope);
			}
		)
	);
	ret &= add_condition(
		"badboy",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					// TODO - multiply argument by infamy_containment_limit during parsing rather than during every execution?
					return current_scope->get_infamy() >= argument * instance_manager.get_definition_manager()
						.get_define_manager().get_country_defines().get_infamy_containment_limit();
				}
			)
		)
	);
	ret &= add_condition(
		"big_producer",
		_parse_condition_node_value_callback<GoodDefinition const*, COUNTRY>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_cast_argument_callback<GoodDefinition const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, GoodDefinition const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, GoodDefinition const* argument
				) -> bool {
					// TODO - check if *current_scope is big producer of *argument
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"blockade",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					// TODO - check if proportion of *current_scope's ports that are blockaded is >= argument
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"brigades_compare",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		// TODO - what does this compare against? current scope vs this scope, or a previous/outer current country scope?
		// brigades_compare first checks if there is a country in THIS scope, if not, then it checks for FROM
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"can_build_factory_in_capital_state",
		_parse_condition_node_value_callback<BuildingType const*, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"can_build_fort_in_capital",
		_parse_condition_node_value_callback<std::pair<bool, bool>, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"can_build_railway_in_capital",
		_parse_condition_node_value_callback<std::pair<bool, bool>, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"can_nationalize",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"can_create_vassals",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"capital",
		_parse_condition_node_value_callback<ProvinceDefinition const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<ProvinceInstance const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, ProvinceInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, ProvinceInstance const* argument
				) -> bool {
					return current_scope->get_capital() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"casus_belli",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					// TODO - check if *current_scope has an active CB against *value
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"check_variable",
		_parse_condition_node_value_callback<std::pair<std::string, fixed_point_t>, COUNTRY>,
		_execute_condition_node_cast_argument_callback<std::pair<std::string, fixed_point_t>, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<
				CountryInstance, scope_t, scope_t, std::pair<std::string, fixed_point_t> const&
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, std::pair<std::string, fixed_point_t> const& argument
				) -> bool {
					return current_scope->get_script_variable(argument.first) >= argument.second;
				}
			)
		)
	);
	ret &= add_condition(
		"civilization_progress",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_civilisation_progress() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"civilized",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->is_civilised() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"colonial_nation",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->get_owns_colonial_province() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"consciousness",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY | POP>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<Pop, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, Pop const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_consciousness() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"constructing_cb",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					// TODO - check if *current_scope is constructing a CB against *value
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"constructing_cb_progress",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					// TODO - check if *current_scope is constructing a CB with progress >= argument
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"constructing_cb_type",
		_parse_condition_node_value_callback<WargoalType const*, COUNTRY>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_cast_argument_callback<WargoalType const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, WargoalType const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, WargoalType const* argument
				) -> bool {
					// TODO - check if *current_scope is constructing a CB with type == argument
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"continent",
		_parse_condition_node_value_callback<Continent const*, COUNTRY | PROVINCE>,
		_execute_condition_node_cast_argument_callback<Continent const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, Continent const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, Continent const* argument
				) -> bool {
					return current_scope->get_province_definition().get_continent() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"controls",
		_parse_condition_node_value_callback<ProvinceDefinition const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<ProvinceInstance const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, ProvinceInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, ProvinceInstance const* argument
				) -> bool {
					return argument->get_controller() == current_scope;
					// TODO - we could double check with: current_scope->has_controlled_province(*argument);
				}
			)
		)
	);
	ret &= add_condition(
		"crime_fighting", // Could be province scope too?
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_administration_spending_slider_value().get_value() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"crime_higher_than_education",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return (
						current_scope->get_administration_spending_slider_value().get_value() >
						current_scope->get_education_spending_slider_value().get_value()
					) == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"crisis_exist",
		// The wiki says this is a COUNTRY scope condition, but I see no reason why it couldn't work globally
		_parse_condition_node_value_callback<bool>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"culture",
		/* TODO - wiki says this can also be used at PROVINCE scope, and even at POP scope it checks that the majority
		 * (or plurality?) of the population in the pop's location province has the specified culture. It says that
		 * has_pop_culture should be used to check a specific pop's culture. The tooltips aren't very clear, with
		 * "culture = <culture>" showing up as "Culture is <culture>" and "has_pop_culture = <culture>" showing up as
		 * "<Pop type plural> in <province> have <culture>". */
		/* Possible explanation:
		 *  - get to PROVINCE scope (ideally starting from a scope <= PROVINCE, so PROVINCE (-> itself) or POP (-> location))
		 *  - convert arg to Culture const*: it'll either be a culture identifier (and so already a Culture const* from its
		 *    parsing), or THIS or FROM ()
		 * */
		_parse_condition_node_value_callback<Culture const*, PROVINCE | POP | THIS | FROM>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"culture_has_union_tag",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					Culture const* primary_culture = current_scope->get_primary_culture();

					if (primary_culture == nullptr) {
						Logger::error(
							"Error executing condition \"", condition.get_identifier(),
							"\": cannot check if country has cultural union country - country \"",
							current_scope->get_identifier(), "\" has no primary culture!"
						);
						return !argument;
					}

					return primary_culture->has_union_country() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"diplomatic_influence",
		_parse_condition_node_who_value_callback,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"education_spending", // TODO - country and province ???
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_education_spending_slider_value().get_value() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"election",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"exists",
		_parse_condition_node_value_callback<
			CountryDefinition const*, COUNTRY | THIS | FROM, _parse_condition_node_special_callback_bool
		>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"government",
		_parse_condition_node_value_callback<GovernmentType const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<GovernmentType const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, GovernmentType const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, GovernmentType const* argument
				) -> bool {
					return current_scope->get_government_type() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"great_wars_enabled",
		// TODO - wiki says this is COUNTRY scope, I see no reason why it can't be global
		_parse_condition_node_value_callback<bool>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, bool argument
			) -> bool {
				return instance_manager.get_politics_instance_manager().get_great_wars_enabled() == argument;
			}
		)
	);
	ret &= add_condition(
		"has_country_flag",
		_parse_condition_node_value_callback<std::string, COUNTRY | PROVINCE>,
		_execute_condition_node_cast_argument_callback<std::string, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, std::string const&>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, std::string const& argument
				) -> bool {
					return current_scope->has_flag(argument);
				}
			)
		)
	);
	ret &= add_condition(
		"has_country_modifier",
		// TODO - which modifiers work here? just country event (+ specially handled debt statics)?
		//      - should this be parsed into a Modifier const* or kept as a std::string?
		_parse_condition_node_value_callback<std::string, COUNTRY | PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"has_cultural_sphere",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"has_leader",
		_parse_condition_node_value_callback<std::string, COUNTRY>,
		_execute_condition_node_cast_argument_callback<std::string, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, std::string const&>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, std::string const& argument
				) -> bool {
					return current_scope->has_leader_with_name(argument);
				}
			)
		)
	);
	ret &= add_condition(
		"has_pop_culture",
		_parse_condition_node_value_callback<Culture const*, COUNTRY | PROVINCE | POP | THIS | FROM>,
		_execute_condition_node_value_or_change_this_or_from_callback<Culture const*>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
				Culture const* argument
			) -> bool {
				struct visitor_t {

					Condition const& condition;
					Culture const* argument;

					bool operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
						return false;
					}

					bool operator()(CountryInstance const* country) const {
						return country->get_culture_proportion(*argument) > 0;
					}
					bool operator()(State const* state) const {
						return state->get_culture_proportion(*argument) > 0;
					}
					bool operator()(ProvinceInstance const* province) const {
						return province->get_culture_proportion(*argument) > 0;
					}
					bool operator()(Pop const* pop) const {
						return &pop->get_culture() == argument;
					}
				};

				return argument != nullptr && std::visit(visitor_t { condition, argument }, current_scope);
			},
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t this_or_from_scope
			) -> Culture const* {
				struct visitor_t {

					Condition const& condition;

					Culture const* operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no THIS/FROM scope!");
						return nullptr;
					}

					constexpr Culture const* operator()(CountryInstance const* country) const {
						return country->get_primary_culture();
					}
					constexpr Culture const* operator()(State const* state) const {
						CountryInstance const* owner = state->get_owner();
						return owner != nullptr ? (*this)(owner) : nullptr;
					}
					constexpr Culture const* operator()(ProvinceInstance const* province) const {
						CountryInstance const* owner = province->get_owner();
						return owner != nullptr ? (*this)(owner) : nullptr;
					}
					constexpr Culture const* operator()(Pop const* pop) const {
						return &pop->get_culture();
					}
				};

				return std::visit(visitor_t { condition }, this_or_from_scope);
			}
		)
	);
	ret &= add_condition(
		"has_pop_religion",
		_parse_condition_node_value_callback<Religion const*, COUNTRY | PROVINCE | POP | THIS | FROM>,
		_execute_condition_node_value_or_change_this_or_from_callback<Religion const*>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
				Religion const* argument
			) -> bool {
				struct visitor_t {

					Condition const& condition;
					Religion const* argument;

					bool operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
						return false;
					}

					bool operator()(CountryInstance const* country) const {
						return country->get_religion_proportion(*argument) > 0;
					}
					bool operator()(State const* state) const {
						return state->get_religion_proportion(*argument) > 0;
					}
					bool operator()(ProvinceInstance const* province) const {
						return province->get_religion_proportion(*argument) > 0;
					}
					bool operator()(Pop const* pop) const {
						return &pop->get_religion() == argument;
					}
				};

				return argument != nullptr && std::visit(visitor_t { condition, argument }, current_scope);
			},
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t this_or_from_scope
			) -> Religion const* {
				struct visitor_t {

					Condition const& condition;

					Religion const* operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no THIS/FROM scope!");
						return nullptr;
					}

					constexpr Religion const* operator()(CountryInstance const* country) const {
						return country->get_religion();
					}
					constexpr Religion const* operator()(State const* state) const {
						CountryInstance const* owner = state->get_owner();
						return owner != nullptr ? (*this)(owner) : nullptr;
					}
					constexpr Religion const* operator()(ProvinceInstance const* province) const {
						CountryInstance const* owner = province->get_owner();
						return owner != nullptr ? (*this)(owner) : nullptr;
					}
					constexpr Religion const* operator()(Pop const* pop) const {
						return &pop->get_religion();
					}
				};

				return std::visit(visitor_t { condition }, this_or_from_scope);
			}
		)
	);
	ret &= add_condition(
		"has_pop_type",
		_parse_condition_node_value_callback<PopType const*, COUNTRY | PROVINCE | POP>,
		_execute_condition_node_cast_argument_callback<PopType const*, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, PopType const* argument
			) -> bool {
				struct visitor_t {

					Condition const& condition;
					PopType const* argument;

					bool operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
						return false;
					}

					bool operator()(CountryInstance const* country) const {
						return country->get_pop_type_proportion(*argument) > 0;
					}
					bool operator()(State const* state) const {
						return state->get_pop_type_proportion(*argument) > 0;
					}
					bool operator()(ProvinceInstance const* province) const {
						return province->get_pop_type_proportion(*argument) > 0;
					}
					bool operator()(Pop const* pop) const {
						return pop->get_type() == argument;
					}
				};

				return argument != nullptr && std::visit(visitor_t { condition, argument }, current_scope);
			}
		)
	);
	ret &= add_condition(
		// checks if the country has lost a war in the last 5 years (losing = accepting a concession offer,
		// even giving white peace (by clicking on the "offer" tab) counts as losing)
		"has_recently_lost_war",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return (
						(instance_manager.get_today() - current_scope->get_last_war_loss_date()) < RECENT_WAR_LOSS_TIME_LIMIT
					) == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"has_unclaimed_cores",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					// Ignores argument, always returning false if the country has unowned cores, otherwise true
					return !current_scope->get_has_unowned_cores();
				}
			)
		)
	);
	// "ideology = <ideology>"" doesn't seem to work as a country condition?
	// NOT FOUND - ret &= add_condition("ideology", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, IDEOLOGY);
	ret &= add_condition(
		"industrial_score",
		// This doesn't seem to work with regular country identifiers, they're treated as 0
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY | THIS | FROM>,
		_execute_condition_node_value_or_change_this_or_from_callback_convert<fixed_point_t, CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					fixed_point_t argument
				) -> bool {
					return current_scope->get_industrial_power() >= argument;
				}
			),
			[](
				Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* this_or_from_scope
			) -> fixed_point_t {
				return this_or_from_scope->get_industrial_power();
			}
		)
	);
	ret &= add_condition(
		"in_sphere",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					// TODO - check if *current_scope is in *value's sphere
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"in_default",
		// The wiki says this can take a bool to check if the scope country is in default or not, but the tooltip always expects a country
		// Other conditions like is_cultural_union seem to work with bools even though they always use the country tooltip,
		// but "in_default = yes" and "in_default = no" seem to be always both false
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"invention",
		_parse_condition_node_value_callback<Invention const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<Invention const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, Invention const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, Invention const* argument
				) -> bool {
					return current_scope->is_invention_unlocked(*argument);
				}
			)
		)
	);
	ret &= add_condition(
		"involved_in_crisis",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_claim_crisis",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_colonial_crisis",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		// TODO - Unused trigger, but is recognized as a valid trigger (does it ever return true?)
		"is_influence_crisis",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_cultural_union",
		_parse_condition_node_value_callback<
			CountryDefinition const*, COUNTRY | THIS | FROM, _parse_condition_node_special_callback_bool
		>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_disarmed",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->is_disarmed() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"is_greater_power",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->is_great_power() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"is_colonial",
		_parse_condition_node_value_callback<bool, STATE | PROVINCE>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<State, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, State const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					/* TODO - ensure province colony status is set properly (default state for owned provinces, default colony
					 *        for unowned) and that every province belongs to a state (land provinces at least, water provinces
					 *        might need special handling) and that all provinces in each state share its colony status
					 *        (we may need to create multiple states for a single country in a single region if it somehow has
					 *        a mismatch of colony statuses and maybe slave statuses too). These same issues apply to other
					 *        conditions mixing corresponding variables at province and state scopes.
					 *      - we could also convert, to Province scope rather than State scope here, however that would force
					 *        State scope uses of this condition to begin a loop over all its provinces (admittedly exiting
					 *        after the first in the true-result case, but needing to go through every one of the state's
					 *        provinces in the false-result case despite it being certain that all of the provinces being
					 *        looped over must have the same colony status (as they're in the same State). */

					return current_scope->is_colonial_state() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"is_core",
		// TODO - ensure ProvinceDefinition const* can only be used at COUNTRY scope and CountryDefinition const* at PROVINCE scope
		_parse_condition_node_value_callback<
			ProvinceDefinition const*, COUNTRY | PROVINCE | THIS | FROM, _parse_condition_node_special_callback_country
		>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_culture_group",
		_parse_condition_node_value_callback<
			CultureGroup const*, COUNTRY | POP | THIS | FROM, _parse_condition_node_special_callback_country
		>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_ideology_enabled",
		// The wiki says this can only be used at COUNTRY and PROVINCE scopes but I see no reason why it can't be global
		_parse_condition_node_value_callback<Ideology const*>,
		_execute_condition_node_cast_argument_callback<Ideology const*, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, Ideology const* argument
			) -> bool {
				return instance_manager.get_politics_instance_manager().is_ideology_unlocked(*argument);
			}
		)
	);
	ret &= add_condition(
		"is_independant", // paradox typo
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_liberation_crisis",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_mobilised",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->is_mobilised() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"is_next_reform",
		_parse_condition_node_value_callback<Reform const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<Reform const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, Reform const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, Reform const* argument
				) -> bool {
					ReformGroup const& reform_group = argument->get_reform_group();
					Reform const* current_reform = current_scope->get_reforms()[reform_group];

					// TODO - how should we hanadle current_reform == nullptr? Always false or true, or true if ordinal == 0?

					return current_reform != nullptr && current_reform->get_ordinal() + 1 == argument->get_ordinal();
				}
			)
		)
	);
	ret &= add_condition(
		"is_our_vassal",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					// TODO - check if *value is a vassal of *current_scope
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"is_possible_vassal",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_releasable_vassal",
		// Could also be country tag but not used in vanilla or mods + tooltip doesn't display
		_parse_condition_node_value_callback<bool, COUNTRY | THIS | FROM>,
		_execute_condition_node_value_or_this_or_from_callback<bool>(
			_execute_condition_node_convert_scope<CountryInstance, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					bool argument
				) -> bool {
					return current_scope->is_releasable_vassal() == argument;
				}
			),
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
				scope_t this_or_from_scope
			) -> bool {
				return _execute_condition_node_convert_scope<CountryInstance>(
					[](
						Condition const& condition, InstanceManager const& instance_manager,
						CountryInstance const* this_or_from_scope
					) -> bool {
						return this_or_from_scope->is_releasable_vassal();
					}
				)(condition, instance_manager, this_or_from_scope);
			}
		)
	);
	ret &= add_condition(
		"is_secondary_power",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->is_secondary_power() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"is_sphere_leader_of",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					// TODO - check if *value is in the sphere of *current_scope
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"is_substate",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_vassal",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"literacy",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY | PROVINCE | POP>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, fixed_point_t argument
			) -> bool {
				struct visitor_t {

					Condition const& condition;
					fixed_point_t const& argument;

					bool operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
						return false;
					}

					constexpr bool operator()(CountryInstance const* country) const {
						return country->get_national_literacy() >= argument;
					}
					constexpr bool operator()(State const* state) const {
						return state->get_average_literacy() >= argument;
					}
					constexpr bool operator()(ProvinceInstance const* province) const {
						return province->get_average_literacy() >= argument;
					}
					constexpr bool operator()(Pop const* pop) const {
						return pop->get_literacy() >= argument;
					}
				};

				return std::visit(visitor_t { condition, argument }, current_scope);
			}
		)
	);
	ret &= add_condition(
		"lost_national",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_owned_cores_controlled_proportion() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"military_access",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					return instance_manager.get_country_relation_manager().get_has_military_access_to(current_scope, value);
				}
			)
		)
	);
	ret &= add_condition(
		"military_score",
		// This doesn't seem to work with regular country identifiers, they're treated as 0
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY | THIS | FROM>,
		_execute_condition_node_value_or_change_this_or_from_callback_convert<fixed_point_t, CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					fixed_point_t argument
				) -> bool {
					return current_scope->get_military_power() >= argument;
				}
			),
			[](
				Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* this_or_from_scope
			) -> fixed_point_t {
				return this_or_from_scope->get_military_power();
			}
		)
	);
	ret &= add_condition(
		"militancy",
		_parse_condition_node_value_callback<fixed_point_t, POP>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<Pop, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, Pop const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_militancy() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"military_spending",
		// TODO - The wiki says this also works for provinces
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_military_spending_slider_value().get_value() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"money",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY | POP>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, fixed_point_t argument
			) -> bool {
				struct visitor_t {

					Condition const& condition;
					fixed_point_t const& argument;

					bool operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
						return false;
					}

					bool operator()(CountryInstance const* country) const {
						return country->get_cash_stockpile().get_copy_of_value() >= argument;
					}
					constexpr bool operator()(State const* state) const {
						CountryInstance const* owner = state->get_owner();
						return owner != nullptr && (*this)(owner);
					}
					constexpr bool operator()(ProvinceInstance const* province) const {
						CountryInstance const* owner = province->get_owner();
						return owner != nullptr && (*this)(owner);
					}
					bool operator()(Pop const* pop) const {
						return pop->get_cash().get_copy_of_value() >= argument;
					}
				};

				return std::visit(visitor_t { condition, argument }, current_scope);
			}
		)
	);
	ret &= add_condition(
		"nationalvalue",
		_parse_condition_node_value_callback<NationalValue const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<NationalValue const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, NationalValue const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, NationalValue const* argument
				) -> bool {
					// TODO - what to do in nullptr NationalValue case?
					return current_scope->get_national_value() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"national_provinces_occupied",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_occupied_provinces_proportion() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		// This uses the British spelling, unlike the "any_neighbor_[country|province]" conditions which use the US spelling
		"neighbour",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					return current_scope->is_neighbour(*value);
				}
			)
		)
	);
	ret &= add_condition(
		"num_of_allies",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"num_of_cities",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, integer_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, integer_t argument
				) -> bool {
					return current_scope->get_owned_provinces().size() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		// Home ports, specifically
		"num_of_ports",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"num_of_revolts",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"number_of_states",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, integer_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, integer_t argument
				) -> bool {
					return static_cast<integer_t>(current_scope->get_states().size()) >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"num_of_substates",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"num_of_vassals",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"num_of_vassals_no_substates",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"owns",
		_parse_condition_node_value_callback<ProvinceDefinition const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<ProvinceInstance const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, ProvinceInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, ProvinceInstance const* argument
				) -> bool {
					return argument->get_owner() == current_scope;
				}
			)
		)
	);
	ret &= add_condition(
		"part_of_sphere",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"plurality",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_plurality() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"political_movement_strength",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		// TODO - reform desire / movement_support_uh_factor * non-colonial-population
		"political_reform_want",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY | POP>,
		_execute_condition_node_unimplemented_no_error
	);
	ret &= add_condition(
		"pop_majority_culture",
		_parse_condition_node_value_callback<Culture const*, COUNTRY | PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"pop_majority_ideology",
		_parse_condition_node_value_callback<Ideology const*, COUNTRY | PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"pop_majority_religion",
		_parse_condition_node_value_callback<Religion const*, COUNTRY | PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"pop_militancy",
		// The wiki also says this can be used at COUNTRY scope
		_parse_condition_node_value_callback<fixed_point_t, PROVINCE>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<Pop, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, Pop const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_militancy() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"prestige",
		// This doesn't seem to work with regular country identifiers, they're treated as 0
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY | THIS | FROM>,
		_execute_condition_node_value_or_change_this_or_from_callback_convert<fixed_point_t, CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					fixed_point_t argument
				) -> bool {
					return current_scope->get_prestige() >= argument;
				}
			),
			[](
				Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* this_or_from_scope
			) -> fixed_point_t {
				return this_or_from_scope->get_prestige();
			}
		)
	);
	ret &= add_condition(
		"primary_culture",
		_parse_condition_node_value_callback<Culture const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<Culture const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, Culture const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, Culture const* argument
				) -> bool {
					// TODO - is it safe to assume argument != nullptr here?
					return current_scope->is_primary_culture(*argument);
				}
			)
		)
	);
	ret &= add_condition(
		"accepted_culture",
		_parse_condition_node_value_callback<Culture const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<Culture const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, Culture const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, Culture const* argument
				) -> bool {
					// TODO - is it safe to assume argument != nullptr here?
					return current_scope->has_accepted_culture(*argument);
				}
			)
		)
	);
	ret &= add_condition(
		"produces",
		_parse_condition_node_value_callback<GoodDefinition const*, COUNTRY | PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"rank",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, integer_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, integer_t argument
				) -> bool {
					return current_scope->get_total_rank() <= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"rebel_power_fraction",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"recruited_percentage",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"relation",
		_parse_condition_node_who_value_callback,
		_execute_condition_node_cast_argument_callback<
			std::pair<CountryDefinition const*, fixed_point_t>, scope_t, scope_t, scope_t
		>(
			_execute_condition_node_convert_scope<
				CountryInstance, scope_t, scope_t, std::pair<CountryDefinition const*, fixed_point_t> const&
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, std::pair<CountryDefinition const*, fixed_point_t> const& argument
				) -> bool {
					return instance_manager.get_country_relation_manager().get_country_relation(
						current_scope,
						&instance_manager.get_country_instance_manager().get_country_instance_from_definition(*argument.first)
					) >= argument.second;
				}
			)
		)
	);
	ret &= add_condition(
		"religion",
		_parse_condition_node_value_callback<Religion const*, COUNTRY | POP | THIS | FROM>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"revanchism",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					// TODO - work out exactly what scale is used for country revanchism and the condition's argument
					// (condition tooltips treat it as a raw decimal, but it's always used as if it's a proportion out of 1)
					return current_scope->get_revanchism() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"revolt_percentage",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	// Doesn't appear in vanilla or test mods
	// ret &= add_condition("rich_tax_above_poor", BOOLEAN, COUNTRY);
	ret &= add_condition(
		"ruling_party",
		_parse_condition_node_value_callback<std::string, COUNTRY>,
		_execute_condition_node_cast_argument_callback<std::string, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, std::string const&>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, std::string const& argument
				) -> bool {
					CountryParty const* ruling_party = current_scope->get_ruling_party();
					return ruling_party != nullptr && ruling_party->get_identifier() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"ruling_party_ideology",
		_parse_condition_node_value_callback<Ideology const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<Ideology const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, Ideology const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, Ideology const* argument
				) -> bool {
					CountryParty const* ruling_party = current_scope->get_ruling_party();
					return ruling_party != nullptr && ruling_party->get_ideology() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"social_movement_strength",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"social_reform_want",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY | POP>,
		_execute_condition_node_unimplemented_no_error
	);
	ret &= add_condition(
		"social_spending",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_social_spending_slider_value().get_value() >= argument;
				}
			)
		)
	);
	// Doesn't seem to work? It shows up as an unlocalised condition and is false even when the argument country has no army at all
	// ret &= add_condition("stronger_army_than", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition(
		"substate_of",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					// TODO - check if *current_scope is a substate of *value
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"tag",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					return current_scope == value;
				}
			)
		)
	);
	ret &= add_condition(
		"tech_school",
		_parse_condition_node_value_callback<TechnologySchool const*, COUNTRY>,
		_execute_condition_node_cast_argument_callback<TechnologySchool const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, TechnologySchool const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, TechnologySchool const* argument
				) -> bool {
					// TODO - what to do in nullptr TechnologySchool case?
					return current_scope->get_tech_school() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"this_culture_union",
		_parse_condition_node_value_callback<
			CountryDefinition const*, COUNTRY | THIS | FROM, _parse_condition_node_special_callback_keyword<this_union_keyword>
		>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		// Divisions = armies with at least 2 units
		"total_amount_of_divisions",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, integer_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, integer_t argument
				) -> bool {
					return current_scope->get_multi_unit_army_count() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"total_amount_of_ships",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, integer_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, integer_t argument
				) -> bool {
					return current_scope->get_ship_count() >= argument;
				}
			)
		)
	);
	// Doesn't show up or work in allow decision tooltips
	// ret &= add_condition("total_defensives", INTEGER, COUNTRY);
	ret &= add_condition(
		"total_num_of_ports",
		_parse_condition_node_value_callback<integer_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, integer_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, integer_t argument
				) -> bool {
					return current_scope->get_port_count() >= argument;
				}
			)
		)
	);
	// Doesn't show up or work in allow decision tooltips
	// ret &= add_condition("total_of_ours_sunk", INTEGER, COUNTRY);
	// TODO - Does this work on Province and Pop scope?, I'll assume yes for now. (Nation and State scopes do work)
	ret &= add_condition(
		"total_pops",
		_parse_condition_node_value_callback<integer_t, COUNTRY | PROVINCE>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope,
				scope_t from_scope, integer_t argument
			) -> bool {
				struct visitor_t {

					Condition const& condition;
					integer_t const& argument;

					bool operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
						return false;
					}

					constexpr bool operator()(CountryInstance const* country) const {
						return country->get_total_population() >= argument;
					}
					constexpr bool operator()(State const* state) const {
						return state->get_total_population() >= argument;
					}
					constexpr bool operator()(ProvinceInstance const* province) const {
						return province->get_total_population() >= argument;
					}
					constexpr bool operator()(Pop const* pop) const {
						return pop->get_size() >= argument;
					}
				};

				return std::visit(visitor_t { condition, argument }, current_scope);
			}
		)
	);
	// Doesn't show up or work in allow decision tooltips
	// ret &= add_condition("total_sea_battles", INTEGER, COUNTRY);
	// Doesn't show up or work in allow decision tooltips
	// ret &= add_condition("total_sunk_by_us", INTEGER, COUNTRY);
	ret &= add_condition(
		// Doesn't show up or work in game, despite being used in vanilla
		"treasury",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"truce_with",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					// TODO - check if *current_scope has a truce with *value
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"unemployment",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY | PROVINCE | POP>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
				scope_t this_scope, scope_t from_scope, fixed_point_t argument
			) -> bool {
				struct visitor_t {

					Condition const& condition;
					fixed_point_t const& argument;

					bool operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
						return false;
					}

					constexpr bool operator()(CountryInstance const* country) const {
						return country->get_unemployment_fraction() >= argument;
					}
					constexpr bool operator()(State const* state) const {
						return state->get_unemployment_fraction() >= argument;
					}
					constexpr bool operator()(ProvinceInstance const* province) const {
						return province->get_unemployment_fraction() >= argument;
					}
					constexpr bool operator()(Pop const* pop) const {
						return pop->get_unemployment_fraction() >= argument;
					}
				};

				return std::visit(visitor_t { condition, argument }, current_scope);
			}
		)
	);
	// Shows up in allow decision tooltips but is always false
	// TODO - "unit_has_leader = no" is the same as "unit_has_leader = yes"
	// ret &= add_condition("unit_has_leader", BOOLEAN, COUNTRY);
	ret &= add_condition(
		"unit_in_battle",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"upper_house",
		_parse_condition_node_value_callback<std::pair<Ideology const*, fixed_point_t>, COUNTRY>,
		_execute_condition_node_cast_argument_callback<std::pair<Ideology const*, fixed_point_t>, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<
				CountryInstance, scope_t, scope_t, std::pair<Ideology const*, fixed_point_t> const&
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, std::pair<Ideology const*, fixed_point_t> const& argument
				) -> bool {
					return current_scope->get_upper_house()[*argument.first] >= argument.second;
				}
			)
		)
	);
	ret &= add_condition(
		"vassal_of",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					// TODO - check if *current_scope is a vassal of *value
					return false;
				}
			)
		)*/
	);
	ret &= add_condition(
		"war",
		_parse_condition_node_value_callback<bool, COUNTRY>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->is_at_war() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"war_exhaustion",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_war_exhaustion() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"war_score",
		_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"war_with",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_unimplemented
		/*_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<CountryInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, CountryInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					// TODO - check if *current_scope is at war with *value
					return false;
				}
			)
		)*/
	);
	// TODO - Undocumented, only seen used in PDM and Victoria 1.02 as "someone_can_form_union_tag = FROM", recognized by the game engine as valid
	ret &= add_condition(
		"someone_can_form_union_tag",
		_parse_condition_node_value_callback<CountryDefinition const*, COUNTRY | THIS | FROM>,
		_execute_condition_node_unimplemented
	);

	/* State Scope Conditions */
	ret &= add_condition(
		"controlled_by",
		_parse_condition_node_value_callback<
			CountryDefinition const*, PROVINCE | THIS | FROM, _parse_condition_node_special_callback_keyword<owner_keyword>
		>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"empty",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					// TODO - Should we have a dedicated "is_uncolonised" function for provinces?
					return (current_scope->get_owner() == nullptr) == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"flashpoint_tension",
		_parse_condition_node_value_callback<fixed_point_t, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"has_building",
		_parse_condition_node_value_callback<
			BuildingType const*, PROVINCE, _parse_condition_node_special_callback_building_type_type
		>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"has_factories",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"has_flashpoint",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	// TODO - I vaguely remember this also applying to states, eh, probably nothing to worry about (for now)
	ret &= add_condition(
		"is_slave",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->get_slave() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"owned_by",
		_parse_condition_node_value_callback<CountryDefinition const*, PROVINCE | THIS | FROM>,
		_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<ProvinceInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					return current_scope->get_owner() == value;
				}
			)
		)
	);
	ret &= add_condition(
		"trade_goods_in_state",
		_parse_condition_node_value_callback<GoodDefinition const*, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"work_available",
		_parse_condition_node_value_callback<std::vector<PopType const*>, PROVINCE>,
		_execute_condition_node_unimplemented
	);

	/* Province Scope Conditions */
	ret &= add_condition(
		"can_build_factory",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"controlled_by_rebels",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"country_units_in_province",
		_parse_condition_node_value_callback<CountryDefinition const*, PROVINCE | THIS | FROM>,
		_execute_condition_node_value_or_convert_this_or_from_callback<CountryInstance>(
			_execute_condition_node_convert_scope<ProvinceInstance, CountryInstance const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					CountryInstance const* value
				) -> bool {
					using enum UnitType::branch_t;
					const auto has_units = [current_scope, value]<UnitType::branch_t Branch>() -> bool {
						for (
							UnitInstanceGroupBranched<Branch> const* unit : current_scope->get_unit_instance_groups<Branch>()
						) {
							if (unit->get_country() == value) {
								return true;
							}
						}
						return false;
					};
					return has_units.template operator()<LAND>() || has_units.template operator()<NAVAL>();
				}
			)
		)
	);
	// Doesn't appear in vanilla or any test mods
	// ret &= add_condition("country_units_in_state", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition(
		"has_crime",
		_parse_condition_node_value_callback<Crime const*, PROVINCE>,
		_execute_condition_node_cast_argument_callback<Crime const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, Crime const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, Crime const* argument
				) -> bool {
					return current_scope->get_crime() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"has_culture_core",
		// This needs a POP and a PROVINCE, not just the POP's location, e.g. migration_target uses this
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"has_empty_adjacent_province",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					// Argument is ignored, always acts as "has_empty_adjacent_province = yes"
					return current_scope->get_has_empty_adjacent_province();
				}
			)
		)
	);
	// Doesn't appear in vanilla or any test mods
	// ret &= add_condition("has_empty_adjacent_state", BOOLEAN, PROVINCE);
	ret &= add_condition(
		"has_national_minority",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	// TODO - Usage returns false always - warn about it?
	ret &= add_condition(
		"has_province_flag",
		_parse_condition_node_value_callback<std::string, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"has_province_modifier",
		// TODO - parse as Modifier const*? Depends what types of modifiers can go here
		_parse_condition_node_value_callback<std::string, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"has_recent_imigration", // paradox typo
		_parse_condition_node_value_callback<integer_t, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_blockaded",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_accepted_culture",
		_parse_condition_node_value_callback<
			CountryDefinition const*, PROVINCE | POP | THIS | FROM, _parse_condition_node_special_callback_bool
		>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_capital",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					CountryInstance const* owner = current_scope->get_owner();
					return (owner != nullptr && owner->get_capital() == current_scope) == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"is_coastal",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->get_province_definition().is_coastal() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"is_overseas",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->get_is_overseas() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"is_primary_culture",
		_parse_condition_node_value_callback<
			CountryDefinition const*, PROVINCE | POP | THIS | FROM, _parse_condition_node_special_callback_bool
		>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"is_state_capital",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					State const* state = current_scope->get_state();
					return (state != nullptr && state->get_capital() == current_scope) == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"is_state_religion",
		_parse_condition_node_value_callback<bool, PROVINCE | POP>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			[](
				Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
				scope_t this_scope, scope_t from_scope, bool argument
			) -> bool {
				struct visitor_t {

					Condition const& condition;
					bool const& argument;

					bool operator()(no_scope_t no_scope) const {
						Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
						return false;
					}

					constexpr bool operator()(CountryInstance const* country) const {
						return (country->get_largest_religion() == country->get_religion()) == argument;
					}
					constexpr bool operator()(State const* state) const {
						CountryInstance const* owner = state->get_owner();
						return (owner != nullptr && state->get_largest_religion() == owner->get_religion()) == argument;
					}
					constexpr bool operator()(ProvinceInstance const* province) const {
						CountryInstance const* owner = province->get_owner();
						return (owner != nullptr && province->get_largest_religion() == owner->get_religion()) == argument;
					}
					constexpr bool operator()(Pop const* pop) const {
						CountryInstance const* owner = pop->get_location()->get_owner();
						return (owner != nullptr && &pop->get_religion() == owner->get_religion()) == argument;
					}
				};

				return std::visit(visitor_t { condition, argument }, current_scope);
			}
		)
	);
	ret &= add_condition(
		"life_rating",
		_parse_condition_node_value_callback<fixed_point_t, PROVINCE>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_life_rating() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"minorities",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->get_has_unaccepted_pops() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"port",
		_parse_condition_node_value_callback<bool, PROVINCE>,
		_execute_condition_node_cast_argument_callback<bool, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, bool>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, bool argument
				) -> bool {
					return current_scope->get_province_definition().has_port() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"province_control_days",
		_parse_condition_node_value_callback<integer_t, PROVINCE>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, integer_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, integer_t argument
				) -> bool {
					return current_scope->get_occupation_duration() >= argument;
				}
			)
		)
	);
	ret &= add_condition(
		"province_id",
		_parse_condition_node_value_callback<ProvinceDefinition const*, PROVINCE>,
		_execute_condition_node_cast_argument_callback<ProvinceDefinition const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, ProvinceDefinition const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, ProvinceDefinition const* argument
				) -> bool {
					return &current_scope->get_province_definition() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"region",
		_parse_condition_node_value_callback<Region const*, PROVINCE>,
		_execute_condition_node_cast_argument_callback<Region const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, Region const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, Region const* argument
				) -> bool {
					return current_scope->get_province_definition().get_region() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"state_id",
		_parse_condition_node_value_callback<ProvinceDefinition const*, PROVINCE>,
		_execute_condition_node_cast_argument_callback<ProvinceDefinition const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, ProvinceDefinition const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, ProvinceDefinition const* argument
				) -> bool {
					return current_scope->get_state() ==
						instance_manager.get_map_instance().get_province_instance_from_definition(*argument).get_state();
				}
			)
		)
	);
	ret &= add_condition(
		"terrain",
		_parse_condition_node_value_callback<TerrainType const*, PROVINCE | POP>, // Why POP?
		_execute_condition_node_cast_argument_callback<TerrainType const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, TerrainType const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, TerrainType const* argument
				) -> bool {
					return current_scope->get_terrain_type() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"trade_goods",
		_parse_condition_node_value_callback<GoodDefinition const*, PROVINCE>,
		_execute_condition_node_cast_argument_callback<GoodDefinition const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, GoodDefinition const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, GoodDefinition const* argument
				) -> bool {
					return current_scope->get_rgo_good() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"unemployment_by_type",
		_parse_condition_node_value_callback<std::pair<PopType const*, fixed_point_t>, PROVINCE>,
		_execute_condition_node_cast_argument_callback<std::pair<PopType const*, fixed_point_t>, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<
				ProvinceInstance, scope_t, scope_t, std::pair<PopType const*, fixed_point_t> const&
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, std::pair<PopType const*, fixed_point_t> const& argument
				) -> bool {
					return current_scope->get_pop_type_unemployed(*argument.first) >=
						argument.second * current_scope->get_pop_type_proportion(*argument.first);
				}
			)
		)
	);
	ret &= add_condition(
		"units_in_province", // Counts number of land regiments, including those being transported by navies
		_parse_condition_node_value_callback<integer_t, PROVINCE>,
		_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<ProvinceInstance, scope_t, scope_t, integer_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, ProvinceInstance const* current_scope,
					scope_t this_scope, scope_t from_scope, integer_t argument
				) -> bool {
					return current_scope->get_land_regiment_count() >= argument;
				}
			)
		)
	);

	/* Pop Scope Conditions */
	ret &= add_condition(
		"agree_with_ruling_party",
		_parse_condition_node_value_callback<fixed_point_t, POP>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"cash_reserves", // argument represents a percentage of daily needs rather than absolute value of £
		_parse_condition_node_value_callback<fixed_point_t, POP>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"life_needs",
		_parse_condition_node_value_callback<fixed_point_t, POP>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<Pop, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, Pop const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_life_needs_fulfilled() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"everyday_needs",
		_parse_condition_node_value_callback<fixed_point_t, POP>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<Pop, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, Pop const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_everyday_needs_fulfilled() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"luxury_needs",
		_parse_condition_node_value_callback<fixed_point_t, POP>,
		_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<Pop, scope_t, scope_t, fixed_point_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, Pop const* current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					return current_scope->get_luxury_needs_fulfilled() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"political_movement",
		_parse_condition_node_value_callback<bool, POP>,
		_execute_condition_node_unimplemented
	);
	// Doesn't appear in vanilla or any test mods
	// ret &= add_condition("pop_majority_issue", IDENTIFIER, POP, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	ret &= add_condition(
		"pop_type",
		_parse_condition_node_value_callback<PopType const*, POP>,
		_execute_condition_node_cast_argument_callback<PopType const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<Pop, scope_t, scope_t, PopType const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, Pop const* current_scope,
					scope_t this_scope, scope_t from_scope, PopType const* argument
				) -> bool {
					return current_scope->get_type() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"social_movement",
		_parse_condition_node_value_callback<bool, POP>,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"strata",
		_parse_condition_node_value_callback<Strata const*, POP>,
		_execute_condition_node_cast_argument_callback<Strata const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<Pop, scope_t, scope_t, Strata const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, Pop const* current_scope,
					scope_t this_scope, scope_t from_scope, Strata const* argument
				) -> bool {
					return &current_scope->get_type()->get_strata() == argument;
				}
			)
		)
	);
	ret &= add_condition(
		"type",
		_parse_condition_node_value_callback<PopType const*, POP>,
		_execute_condition_node_cast_argument_callback<PopType const*, scope_t, scope_t, scope_t>(
			_execute_condition_node_convert_scope<Pop, scope_t, scope_t, PopType const*>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, Pop const* current_scope,
					scope_t this_scope, scope_t from_scope, PopType const* argument
				) -> bool {
					return current_scope->get_type() == argument;
				}
			)
		)
	);

	for (Strata const& strata : definition_manager.get_pop_manager().get_stratas()) {
		const std::string_view identifier = strata.get_identifier();

		ret &= add_condition(
			StringUtils::append_string_views(identifier, "_tax"),
			_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
			_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
				_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
					[](
						Condition const& condition, InstanceManager const& instance_manager,
						CountryInstance const* current_scope, scope_t this_scope, scope_t from_scope, fixed_point_t argument
					) -> bool {
						Strata const& strata = *static_cast<Strata const*>(condition.get_condition_data());

						return current_scope->get_tax_rate_slider_value_by_strata()[strata].get_value() >= argument;
					}
				)
			),
			&strata
		);
		ret &= add_condition(
			StringUtils::append_string_views(identifier, "_strata_life_needs"),
			_parse_condition_node_value_callback<fixed_point_t, COUNTRY | PROVINCE>,
			_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					Strata const& strata = *static_cast<Strata const*>(condition.get_condition_data());

					struct visitor_t {

						Condition const& condition;
						fixed_point_t const& argument;
						Strata const& strata;

						bool operator()(no_scope_t no_scope) const {
							Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
							return false;
						}

						constexpr bool operator()(CountryInstance const* country) const {
							return country->get_strata_life_needs_fulfilled(strata) >= argument;
						}
						constexpr bool operator()(State const* state) const {
							return state->get_strata_life_needs_fulfilled(strata) >= argument;
						}
						constexpr bool operator()(ProvinceInstance const* province) const {
							return province->get_strata_life_needs_fulfilled(strata) >= argument;
						}
						constexpr bool operator()(Pop const* pop) const {
							return (
								&pop->get_type()->get_strata() == &strata ? pop->get_life_needs_fulfilled() :
									fixed_point_t::_0()
							) >= argument;
						}
					};

					return std::visit(visitor_t { condition, argument, strata }, current_scope);
				}
			),
			&strata
		);
		ret &= add_condition(
			StringUtils::append_string_views(identifier, "_strata_everyday_needs"),
			_parse_condition_node_value_callback<fixed_point_t, COUNTRY | PROVINCE>,
			_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					Strata const& strata = *static_cast<Strata const*>(condition.get_condition_data());

					struct visitor_t {

						Condition const& condition;
						fixed_point_t const& argument;
						Strata const& strata;

						bool operator()(no_scope_t no_scope) const {
							Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
							return false;
						}

						constexpr bool operator()(CountryInstance const* country) const {
							return country->get_strata_everyday_needs_fulfilled(strata) >= argument;
						}
						constexpr bool operator()(State const* state) const {
							return state->get_strata_everyday_needs_fulfilled(strata) >= argument;
						}
						constexpr bool operator()(ProvinceInstance const* province) const {
							return province->get_strata_everyday_needs_fulfilled(strata) >= argument;
						}
						constexpr bool operator()(Pop const* pop) const {
							return (
								&pop->get_type()->get_strata() == &strata ? pop->get_everyday_needs_fulfilled() :
									fixed_point_t::_0()
							) >= argument;
						}
					};

					return std::visit(visitor_t { condition, argument, strata }, current_scope);
				}
			),
			&strata
		);
		ret &= add_condition(
			StringUtils::append_string_views(identifier, "_strata_luxury_needs"),
			_parse_condition_node_value_callback<fixed_point_t, COUNTRY | PROVINCE>,
			_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					Strata const& strata = *static_cast<Strata const*>(condition.get_condition_data());

					struct visitor_t {

						Condition const& condition;
						fixed_point_t const& argument;
						Strata const& strata;

						bool operator()(no_scope_t no_scope) const {
							Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
							return false;
						}

						constexpr bool operator()(CountryInstance const* country) const {
							return country->get_strata_luxury_needs_fulfilled(strata) >= argument;
						}
						constexpr bool operator()(State const* state) const {
							return state->get_strata_luxury_needs_fulfilled(strata) >= argument;
						}
						constexpr bool operator()(ProvinceInstance const* province) const {
							return province->get_strata_luxury_needs_fulfilled(strata) >= argument;
						}
						constexpr bool operator()(Pop const* pop) const {
							return (
								&pop->get_type()->get_strata() == &strata ? pop->get_luxury_needs_fulfilled() :
									fixed_point_t::_0()
							) >= argument;
						}
					};

					return std::visit(visitor_t { condition, argument, strata }, current_scope);
				}
			),
			&strata
		);
		ret &= add_condition(
			StringUtils::append_string_views(identifier, "_strata_militancy"),
			_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
			_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					Strata const& strata = *static_cast<Strata const*>(condition.get_condition_data());

					struct visitor_t {

						Condition const& condition;
						fixed_point_t const& argument;
						Strata const& strata;

						bool operator()(no_scope_t no_scope) const {
							Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
							return false;
						}

						constexpr bool operator()(CountryInstance const* country) const {
							return country->get_strata_militancy(strata) >= argument;
						}
						constexpr bool operator()(State const* state) const {
							return state->get_strata_militancy(strata) >= argument;
						}
						constexpr bool operator()(ProvinceInstance const* province) const {
							return province->get_strata_militancy(strata) >= argument;
						}
						constexpr bool operator()(Pop const* pop) const {
							return (
								&pop->get_type()->get_strata() == &strata ? pop->get_militancy() : fixed_point_t::_0()
							) >= argument;
						}
					};

					return std::visit(visitor_t { condition, argument, strata }, current_scope);
				}
			),
			&strata
		);
	}

	/* Scopes from other registries */
	for (CountryDefinition const& country : definition_manager.get_country_definition_manager().get_country_definitions()) {
		ret &= add_condition(
			country.get_identifier(),
			_parse_condition_node_list_callback<COUNTRY>,
			_execute_condition_node_list_single_scope_callback<expect_true, require_all>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
					scope_t this_scope, scope_t from_scope
				) -> scope_t {
					CountryDefinition const& country = *static_cast<CountryDefinition const*>(condition.get_condition_data());

					return &instance_manager.get_country_instance_manager().get_country_instance_from_definition(country);
				}
			),
			&country
		);
	}

	for (Region const& region : definition_manager.get_map_definition().get_regions()) {
		ret &= add_condition(
			region.get_identifier(),
			_parse_condition_node_list_callback<PROVINCE>,
			_execute_condition_node_list_multi_scope_callback<
				expect_true, require_all, std::vector<ProvinceDefinition const*> const&
			>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
					scope_t this_scope, scope_t from_scope
				) -> std::vector<ProvinceDefinition const*> const& {
					Region const& region = *static_cast<Region const*>(condition.get_condition_data());

					return region.get_provinces();
				}
			),
			&region
		);
	}

	for (ProvinceDefinition const& province : definition_manager.get_map_definition().get_province_definitions()) {
		ret &= add_condition(
			province.get_identifier(),
			_parse_condition_node_list_callback<PROVINCE>,
			_execute_condition_node_list_single_scope_callback<expect_true, require_all>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
					scope_t this_scope, scope_t from_scope
				) -> scope_t {
					ProvinceDefinition const& province =
						*static_cast<ProvinceDefinition const*>(condition.get_condition_data());

					return &instance_manager.get_map_instance().get_province_instance_from_definition(province);
				}
			),
			&province
		);
	}

	/* Conditions from other registries */
	for (
		Technology const& technology : definition_manager.get_research_manager().get_technology_manager().get_technologies()
	) {
		ret &= add_condition(
			technology.get_identifier(),
			// TODO - Could convert integer to bool?
			_parse_condition_node_value_callback<integer_t, COUNTRY>,
			_execute_condition_node_cast_argument_callback<integer_t, scope_t, scope_t, scope_t>(
				_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, integer_t>(
					[](
						Condition const& condition, InstanceManager const& instance_manager,
						CountryInstance const* current_scope, scope_t this_scope, scope_t from_scope, integer_t argument
					) -> bool {
						Technology const& technology = *static_cast<Technology const*>(condition.get_condition_data());

						return current_scope->is_technology_unlocked(technology) == (argument != 0);
					}
				)
			),
			&technology
		);
	}

	for (Ideology const& ideology : definition_manager.get_politics_manager().get_ideology_manager().get_ideologies()) {
		ret &= add_condition(
			ideology.get_identifier(),
			_parse_condition_node_value_callback<fixed_point_t, COUNTRY | PROVINCE>,
			_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
				[](
					Condition const& condition, InstanceManager const& instance_manager, scope_t current_scope,
					scope_t this_scope, scope_t from_scope, fixed_point_t argument
				) -> bool {
					Ideology const& ideology = *static_cast<Ideology const*>(condition.get_condition_data());

					struct visitor_t {

						Condition const& condition;
						fixed_point_t const& argument;
						Ideology const& ideology;

						bool operator()(no_scope_t no_scope) const {
							Logger::error("Error executing condition \"", condition.get_identifier(), "\": no current scope!");
							return false;
						}

						bool operator()(CountryInstance const* country) const {
							return country->get_ideology_support(ideology) >= argument * country->get_total_population();
						}
						bool operator()(State const* state) const {
							return state->get_ideology_support(ideology) >= argument * state->get_total_population();
						}
						bool operator()(ProvinceInstance const* province) const {
							return province->get_ideology_support(ideology) >= argument * province->get_total_population();
						}
						bool operator()(Pop const* pop) const {
							return pop->get_ideology_support(ideology) >= argument * pop->get_size();
						}
					};

					return std::visit(visitor_t { condition, argument, ideology }, current_scope);
				}
			),
			&ideology
		);
	}

	for (IssueGroup const& issue_group : definition_manager.get_politics_manager().get_issue_manager().get_issue_groups()) {
		ret &= add_condition(
			issue_group.get_identifier(),
			// TODO - should we check that the Issue actually belongs to this IssueGroup?
			_parse_condition_node_value_callback<Issue const*, COUNTRY>,
			_execute_condition_node_cast_argument_callback<Issue const*, scope_t, scope_t, scope_t>(
				_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, Issue const*>(
					[](
						Condition const& condition, InstanceManager const& instance_manager,
						CountryInstance const* current_scope, scope_t this_scope, scope_t from_scope, Issue const* argument
					) -> bool {
						IssueGroup const& issue_group = *static_cast<IssueGroup const*>(condition.get_condition_data());

						CountryParty const* ruling_party = current_scope->get_ruling_party();

						return ruling_party != nullptr && ruling_party->get_policies()[issue_group] == argument;
					}
				)
			),
			&issue_group
		);
	}

	for (ReformGroup const& reform_group : definition_manager.get_politics_manager().get_issue_manager().get_reform_groups()) {
		ret &= add_condition(
			reform_group.get_identifier(),
			_parse_condition_node_value_callback<Reform const*, COUNTRY>,
			_execute_condition_node_cast_argument_callback<Reform const*, scope_t, scope_t, scope_t>(
				_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, Reform const*>(
					[](
						Condition const& condition, InstanceManager const& instance_manager,
						CountryInstance const* current_scope, scope_t this_scope, scope_t from_scope, Reform const* argument
					) -> bool {
						ReformGroup const& reform_group = *static_cast<ReformGroup const*>(condition.get_condition_data());

						return current_scope->get_reforms()[reform_group] == argument;
					}
				)
			),
			&reform_group
		);
	}

	for (Issue const& issue : definition_manager.get_politics_manager().get_issue_manager().get_issues()) {
		ret &= add_condition(
			issue.get_identifier(),
			_parse_condition_node_value_callback<fixed_point_t, COUNTRY | PROVINCE>,
			_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
				_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
					[](
						Condition const& condition, InstanceManager const& instance_manager,
						CountryInstance const* current_scope, scope_t this_scope, scope_t from_scope, fixed_point_t argument
					) -> bool {
						Issue const& issue = *static_cast<Issue const*>(condition.get_condition_data());

						return current_scope->get_issue_support(issue) * 100 >=
							argument * current_scope->get_total_population();
					}
				)
			),
			&issue
		);
	}

	for (Reform const& reform : definition_manager.get_politics_manager().get_issue_manager().get_reforms()) {
		ret &= add_condition(
			reform.get_identifier(),
			_parse_condition_node_value_callback<fixed_point_t, COUNTRY | PROVINCE>,
			_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
				_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
					[](
						Condition const& condition, InstanceManager const& instance_manager,
						CountryInstance const* current_scope, scope_t this_scope, scope_t from_scope, fixed_point_t argument
					) -> bool {
						Reform const& reform = *static_cast<Reform const*>(condition.get_condition_data());

						return current_scope->get_issue_support(reform) * 100 >=
							argument * current_scope->get_total_population();
					}
				)
			),
			&reform
		);
	}

	for (PopType const& pop_type : definition_manager.get_pop_manager().get_pop_types()) {
		ret &= add_condition(
			pop_type.get_identifier(),
			_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
			_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
				_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
					[](
						Condition const& condition, InstanceManager const& instance_manager,
						CountryInstance const* current_scope, scope_t this_scope, scope_t from_scope, fixed_point_t argument
					) -> bool {
						PopType const& pop_type = *static_cast<PopType const*>(condition.get_condition_data());

						return current_scope->get_pop_type_proportion(pop_type) >=
							argument * current_scope->get_total_population();
					}
				)
			),
			&pop_type
		);
	}

	for (
		GoodDefinition const& good :
			definition_manager.get_economy_manager().get_good_definition_manager().get_good_definitions()
	) {
		ret &= add_condition(
			good.get_identifier(),
			_parse_condition_node_value_callback<fixed_point_t, COUNTRY>,
			_execute_condition_node_cast_argument_callback<fixed_point_t, scope_t, scope_t, scope_t>(
				_execute_condition_node_convert_scope<CountryInstance, scope_t, scope_t, fixed_point_t>(
					[](
						Condition const& condition, InstanceManager const& instance_manager,
						CountryInstance const* current_scope, scope_t this_scope, scope_t from_scope, fixed_point_t argument
					) -> bool {
						GoodDefinition const& good = *static_cast<GoodDefinition const*>(condition.get_condition_data());

						return current_scope->get_good_data(good).stockpile_amount >= argument;
					}
				)
			),
			&good
		);
	}

	/* Newspaper conditions */
	ret &= add_condition(
		"tags_eq",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"values_eq",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"strings_eq",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"dates_eq",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"tags_greater",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"values_greater",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"strings_greater",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"dates_greater",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"tags_match",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"values_match",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"strings_match",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"dates_match",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"tags_contains",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"values_contains",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"strings_contains",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"dates_contains",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	ret &= add_condition(
		"length_greater",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);
	// Amount printed, NOTE: Seems to be buggy when used on decisions and events
	ret &= add_condition(
		"news_printing_count",
		_parse_condition_node_unimplemented,
		_execute_condition_node_unimplemented
	);

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
