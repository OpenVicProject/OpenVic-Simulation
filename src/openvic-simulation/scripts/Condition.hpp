#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <variant>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/EnumBitfield.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

/*

	ConditionType
	- execute function (taking ConditionValue's Argument + current context, e.g. THIS and FROM)

	ConditionValue
	- ConditionType
	- Arguments:
		- std::vector<ConditionValue>
		- std::string
		- ProvinceDefinition const*
		- CountryDefinition const*
		- THIS or FROM
		- bool
		- int
		- fixed_point_t
		- GovernmentType const*
		- std::string + int (or fixed_point_t?) (for check_variable)
		- CountryDefinition const* + int (or fixed_point_t?) (for diplomatic_influence and relation)
		- GoodDefinition const*
		- BuildingType const*
		- Reform const*
		- ReformGroup const* + Reform const*
		- Issue const*
		- IssueGroup const* Issue const*
		- Issue const* + fixed_point_t
		- WargoalType const*
		- Modifier const*
		- Ideology const*
		- Ideology const* + fixed_point_t (for upper_house)
		- Invention const*
		- Culture const*
		- Religion const*
		- PopType const*
		- PopType const* + fixed_point_t (for unemployment_by_type)
		- NationalValue const*
		- Strata const* + fixed_point_t
		- CountryParty const* (for ruling_party, maybe better to use std::string as parties are stored per country)
		- Continent const*
		- Crime const*
		- Region const*
		- TerrainType const*

*/

namespace OpenVic {

	enum class scope_type_t : uint8_t {
		NO_SCOPE    = 0,
		POP         = 1 << 0,
		PROVINCE    = 1 << 1,
		STATE       = 1 << 2,
		COUNTRY     = 1 << 3,
		THIS        = 1 << 4, // Indicator bit for scope switching ("use the THIS scope", not a scope in and of itself)
		FROM        = 1 << 5, // Indicator bit for scope switching ("use the FROM scope", not a scope in and of itself)
		FULL_SCOPE_MASK   = (1 << 6) - 1, // All possible scope bits (including THIS and FROM)
		ALL_SCOPES = POP | PROVINCE | STATE | COUNTRY // All real scopes (without THIS and FROM)
	};

	/* Allows enum types to be used with bitwise operators. */
	template<> struct enable_bitfield<scope_type_t> : std::true_type {};

	/* Returns true if the values have any bit in common. */
	inline constexpr bool share_scope_type(scope_type_t lhs, scope_type_t rhs) {
		return (lhs & rhs) != scope_type_t::NO_SCOPE;
	}

#define BUILD_STRING(entry) \
	if (share_scope_type(value, entry)) { \
		if (type_found) { \
			stream << " | "; \
		} else { \
			type_found = true; \
		} \
		stream << #entry; \
	}

	inline std::ostream& operator<<(std::ostream& stream, scope_type_t value) {
		using enum scope_type_t;

		if (value == NO_SCOPE) {
			return stream << "[NO_SCOPE]";
		}

		bool type_found = false;

		stream << '[';

		BUILD_STRING(POP);
		BUILD_STRING(PROVINCE);
		BUILD_STRING(COUNTRY);
		BUILD_STRING(THIS);
		BUILD_STRING(FROM);

		if (!type_found) {
			stream << "INVALID SCOPE";
		}

		return stream << ']';
	}

#undef BUILD_STRING

	struct ConditionManager;
	struct ConditionScript;
	struct CountryDefinition;
	struct CountryInstance;
	struct State;
	struct ProvinceDefinition;
	struct ProvinceInstance;
	struct Pop;
	struct GoodDefinition;
	struct ProvinceSetModifier;
	using Continent = ProvinceSetModifier;
	struct BuildingType;
	struct Issue;
	struct WargoalType;
	struct PopType;
	struct CultureGroup;
	struct Culture;
	struct Religion;
	struct GovernmentType;
	struct Ideology;
	struct Reform;
	struct NationalValue;
	struct Invention;
	struct TechnologySchool;
	struct Crime;
	struct Region;
	struct TerrainType;
	struct Strata;
	struct Condition;
	struct DefinitionManager;
	struct InstanceManager;

	struct ConditionNode {
		friend struct ConditionManager;
		friend struct ConditionScript;

		// std::variant's default constructor sets it to the first type in its parameter list, so for argument_t and scope_t
		// a default-constructed instance represents no_argument_t or no_scope_t.

		struct no_argument_t {};
		struct this_argument_t {};
		struct from_argument_t {};
		struct special_argument_t {};
		using integer_t = int64_t;
		using argument_t = std::variant<
			// No argument
			no_argument_t,
			// Script reference arguments
			this_argument_t, from_argument_t,
			// Special argument
			special_argument_t,
			// List argument
			std::vector<ConditionNode>,
			// Value arguments
			bool, std::string, integer_t, fixed_point_t,
			// Game object arguments
			CountryDefinition const*, ProvinceDefinition const*, GoodDefinition const*, Continent const*, BuildingType const*,
			Issue const*, WargoalType const*, PopType const*, CultureGroup const*, Culture const*, Religion const*,
			GovernmentType const*, Ideology const*, Reform const*, NationalValue const*, Invention const*,
			TechnologySchool const*, Crime const*, Region const*, TerrainType const*, Strata const*,
			// Multi-value arguments
			std::pair<PopType const*, fixed_point_t>, std::pair<bool, bool>, std::pair<Ideology const*, fixed_point_t>,
			std::vector<PopType const*>, std::pair<CountryDefinition const*, fixed_point_t>,
			std::pair<this_argument_t, fixed_point_t>, std::pair<from_argument_t, fixed_point_t>,
			std::pair<std::string, fixed_point_t>
		>;

		static constexpr bool is_this_argument(argument_t const& argument) {
			return std::holds_alternative<this_argument_t>(argument);
		}

		static constexpr bool is_from_argument(argument_t const& argument) {
			return std::holds_alternative<from_argument_t>(argument);
		}

		static constexpr bool is_special_argument(argument_t const& argument) {
			return std::holds_alternative<special_argument_t>(argument);
		}

		struct no_scope_t {};
		using scope_t = std::variant<
			no_scope_t,
			CountryInstance const*,
			State const*,
			ProvinceInstance const*,
			Pop const*
		>;

		static constexpr bool is_no_scope(scope_t const& scope) {
			return std::holds_alternative<no_scope_t>(scope);
		}

	private:
		Condition const* PROPERTY(condition);
		argument_t PROPERTY(argument);

		ConditionNode(
			Condition const* new_condition = nullptr, argument_t&& new_argument = no_argument_t {}
		);

	public:
		ConditionNode(ConditionNode&&) = default;
		ConditionNode& operator=(ConditionNode&&) = default;

		constexpr bool is_initialised() const {
			return condition != nullptr;
		}

		bool execute(
			InstanceManager const& instance_manager, scope_t current_scope, scope_t this_scope, scope_t from_scope
		) const;
	};

	// TODO - find a way to stop random things getting turned into arguments and using this, e.g. "size_t"s
	// std::ostream& operator<<(std::ostream& stream, ConditionNode::argument_t const& argument);
	std::string argument_to_string(ConditionNode::argument_t const& argument);
	std::ostream& operator<<(std::ostream& stream, ConditionNode const& node);

	// template<typename... Args>
	// // using callback_t = std::function<bool(Args...)>;
	// using callback_t = NodeTools::callback_t<Args...>;

	struct Condition : HasIdentifier {
		friend struct ConditionManager;

		using parse_callback_t = NodeTools::callback_t<
			// bool(condition, definition_manager, current_scope, this_scope, from_scope, node, callback)
			Condition const&, DefinitionManager const&, scope_type_t, scope_type_t, scope_type_t, ast::NodeCPtr,
			NodeTools::callback_t<ConditionNode::argument_t&&>
		>;
		using execute_callback_t = NodeTools::callback_t<
			// bool(condition, instance_manager, current_scope, this_scope, from_scope, argument)
			Condition const&, InstanceManager const&, ConditionNode::scope_t, ConditionNode::scope_t, ConditionNode::scope_t,
			ConditionNode::argument_t const&
		>;

	private:
		parse_callback_t parse_callback;
		execute_callback_t execute_callback;
		void const* PROPERTY(condition_data);

		Condition(
			std::string_view new_identifier,
			parse_callback_t&& new_parse_callback,
			execute_callback_t&& new_execute_callback,
			void const* new_condition_data
		);

	public:
		Condition(Condition&&) = default;

		parse_callback_t get_parse_callback() const {
			return parse_callback;
		}
		execute_callback_t get_execute_callback() const {
			return execute_callback;
		}
	};

	struct ConditionManager {
	private:
		CaseInsensitiveIdentifierRegistry<Condition> IDENTIFIER_REGISTRY(condition);
		Condition const* PROPERTY(root_condition);

		DefinitionManager const& definition_manager;

		bool add_condition(
			std::string_view identifier,
			Condition::parse_callback_t&& parse_callback,
			Condition::execute_callback_t&& execute_callback,
			void const* condition_data = nullptr
		);

		template<
			scope_type_t CHANGE_SCOPE = scope_type_t::NO_SCOPE,
			scope_type_t ALLOWED_SCOPES = scope_type_t::ALL_SCOPES,
			bool TOP_SCOPE = false
		>
		static bool _parse_condition_node_list_callback(
			Condition const& condition, DefinitionManager const& definition_manager, scope_type_t current_scope,
			scope_type_t this_scope, scope_type_t from_scope, ast::NodeCPtr node,
			NodeTools::callback_t<ConditionNode::argument_t&&> callback
		);

		NodeTools::Callback<Condition const&, ast::NodeCPtr> auto expect_condition_node(
			scope_type_t current_scope, scope_type_t this_scope,
			scope_type_t from_scope, NodeTools::Callback<ConditionNode&&> auto callback
		) const;

		NodeTools::NodeCallback auto expect_condition_node_list_and_length(
			scope_type_t current_scope, scope_type_t this_scope,
			scope_type_t from_scope, NodeTools::Callback<ConditionNode&&> auto callback,
			NodeTools::LengthCallback auto length_callback, bool top_scope = false
		) const;

		NodeTools::NodeCallback auto expect_condition_node_list(
			scope_type_t current_scope, scope_type_t this_scope,
			scope_type_t from_scope, NodeTools::Callback<ConditionNode&&> auto callback, bool top_scope = false
		) const;

	public:
		ConditionManager(DefinitionManager const& new_definition_manager);

		bool setup_conditions();

		// TODO - way to call with a state as the starting scope (or rather the list of provinces making up a state)
		// For: ProductionType bonus trigger, WargoalType allowed_states, allowed_substate_regions and allowed_states_in_crisis

		NodeTools::node_callback_t expect_condition_script(
			scope_type_t initial_scope, scope_type_t this_scope,
			scope_type_t from_scope, NodeTools::callback_t<ConditionNode&&> callback
		) const;
	};
}
