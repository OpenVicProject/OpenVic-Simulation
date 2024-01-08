#pragma once

#include <string_view>
#include <variant>

#include "openvic-simulation/types/EnumBitfield.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct ConditionManager;
	struct ConditionScript;
	struct GameManager;

	enum class value_type_t : uint8_t {
		NO_TYPE     = 0,
		IDENTIFIER  = 1 << 0,
		STRING      = 1 << 1,
		BOOLEAN     = 1 << 2,
		BOOLEAN_INT = 1 << 3,
		INTEGER     = 1 << 4,
		REAL        = 1 << 5,
		COMPLEX     = 1 << 6,
		GROUP       = 1 << 7,
		MAX_VALUE   = (1 << 8) - 1
	};
	template<> struct enable_bitfield<value_type_t> : std::true_type {};

	// Order matters in this enum, for the fallback system to work
	// smaller entities must have smaller integers associated!
	enum class scope_t : uint8_t { //TODO: maybe distinguish TRANSPARENT from NO_SCOPE
		NO_SCOPE    = 0,
		POP         = 1 << 0,
		PROVINCE    = 1 << 1,
		STATE       = 1 << 2,
		COUNTRY     = 1 << 3,
		THIS        = 1 << 4,
		FROM        = 1 << 5,
		MAX_SCOPE   = (1 << 6) - 1
	};
	template<> struct enable_bitfield<scope_t> : std::true_type {};

	enum class identifier_type_t : uint32_t {
		NONE            = 0,
		VARIABLE        = 1 << 0,
		GLOBAL_FLAG     = 1 << 1,
		COUNTRY_FLAG    = 1 << 2,
		PROVINCE_FLAG   = 1 << 3,
		COUNTRY_TAG     = 1 << 4,
		PROVINCE_ID     = 1 << 5,
		REGION          = 1 << 6,
		IDEOLOGY        = 1 << 7,
		REFORM_GROUP    = 1 << 8,
		REFORM          = 1 << 9,
		ISSUE           = 1 << 10,
		POP_TYPE        = 1 << 11,
		POP_STRATA      = 1 << 12,
		TECHNOLOGY      = 1 << 13,
		INVENTION       = 1 << 14,
		TECH_SCHOOL     = 1 << 15,
		CULTURE         = 1 << 16,
		CULTURE_GROUP   = 1 << 17,
		RELIGION        = 1 << 18,
		TRADE_GOOD      = 1 << 19,
		BUILDING        = 1 << 20,
		CASUS_BELLI     = 1 << 21,
		GOVERNMENT_TYPE = 1 << 22,
		MODIFIER        = 1 << 23,
		NATIONAL_VALUE  = 1 << 24,
		CULTURE_UNION   = 1 << 25, // same as COUNTRY_TAG but also accepts scope this_union
		CONTINENT       = 1 << 26,
		CRIME           = 1 << 27,
		TERRAIN         = 1 << 28,
	};
	template<> struct enable_bitfield<identifier_type_t> : std::true_type {};

	/* Returns true if the values have any bit in common. */
	constexpr inline bool share_value_type(value_type_t lhs, value_type_t rhs) {
		return (lhs & rhs) != value_type_t::NO_TYPE;
	}
	constexpr inline bool share_scope(scope_t lhs, scope_t rhs) {
		return (lhs & rhs) != scope_t::NO_SCOPE;
	}
	constexpr inline bool share_identifier_type(identifier_type_t lhs, identifier_type_t rhs) {
		return (lhs & rhs) != identifier_type_t::NONE;
	}

#define _BUILD_STRING(entry, share) if (share(value, entry)) { ret += #entry " | "; }

#define BUILD_STRING(entry) _BUILD_STRING(entry, share_value_type)

	inline std::string get_value_type_string(value_type_t value) {
		using enum value_type_t;
		if (value == NO_TYPE) {
			return "[NO_TYPE]";
		}
		std::string ret = {};
		BUILD_STRING(IDENTIFIER);
		BUILD_STRING(STRING);
		BUILD_STRING(BOOLEAN);
		BUILD_STRING(BOOLEAN_INT);
		BUILD_STRING(INTEGER);
		BUILD_STRING(REAL);
		BUILD_STRING(COMPLEX);
		BUILD_STRING(GROUP);
		return "[" + ret.substr(0, ret.length() - 3) + "]";
	}

#undef BUILD_STRING
#define BUILD_STRING(entry) _BUILD_STRING(entry, share_scope)

	inline std::string get_scope_string(scope_t value) {
		using enum scope_t;
		if (value == NO_SCOPE) {
			return "[NO_SCOPE]";
		}
		std::string ret = {};
		BUILD_STRING(COUNTRY);
		BUILD_STRING(STATE);
		BUILD_STRING(PROVINCE);
		BUILD_STRING(POP);
		BUILD_STRING(THIS);
		BUILD_STRING(FROM);
		return "[" + ret.substr(0, ret.length() - 3) + "]";
	}

#undef BUILD_STRING
#define BUILD_STRING(entry) _BUILD_STRING(entry, share_identifier_type)

	inline std::string get_identifier_type_string(identifier_type_t value) {
		using enum identifier_type_t;
		if (value == NONE) {
			return "[NONE]";
		}
		std::string ret = {};
		BUILD_STRING(VARIABLE);
		BUILD_STRING(GLOBAL_FLAG);
		BUILD_STRING(COUNTRY_FLAG);
		BUILD_STRING(PROVINCE_FLAG);
		BUILD_STRING(COUNTRY_TAG);
		BUILD_STRING(PROVINCE_ID);
		BUILD_STRING(REGION);
		BUILD_STRING(IDEOLOGY);
		BUILD_STRING(REFORM_GROUP);
		BUILD_STRING(REFORM);
		BUILD_STRING(ISSUE);
		BUILD_STRING(POP_TYPE);
		BUILD_STRING(POP_STRATA);
		BUILD_STRING(TECHNOLOGY);
		BUILD_STRING(INVENTION);
		BUILD_STRING(TECH_SCHOOL);
		BUILD_STRING(CULTURE);
		BUILD_STRING(CULTURE_GROUP);
		BUILD_STRING(RELIGION);
		BUILD_STRING(TRADE_GOOD);
		BUILD_STRING(BUILDING);
		BUILD_STRING(CASUS_BELLI);
		BUILD_STRING(GOVERNMENT_TYPE);
		BUILD_STRING(MODIFIER);
		BUILD_STRING(NATIONAL_VALUE);
		BUILD_STRING(CULTURE_UNION);
		BUILD_STRING(CONTINENT);
		BUILD_STRING(CRIME);
		BUILD_STRING(TERRAIN);
		return "[" + ret.substr(0, ret.length() - 3) + "]";
	}

#undef BUILD_STRING
#undef _BUILD_STRING

	struct Condition : HasIdentifier {
		friend struct ConditionManager;
		using enum identifier_type_t;

	private:
		const value_type_t PROPERTY(value_type);
		const scope_t PROPERTY(scope);
		const scope_t PROPERTY(scope_change);
		const identifier_type_t PROPERTY(key_identifier_type);
		const identifier_type_t PROPERTY(value_identifier_type);

		Condition(
			std::string_view new_identifier, value_type_t new_value_type, scope_t new_scope,
			scope_t new_scope_change, identifier_type_t new_key_identifier_type,
			identifier_type_t new_value_identifier_type
		);

	public:
		Condition(Condition&&) = default;
	};

	struct ConditionNode {
		friend struct ConditionManager;
		friend struct ConditionScript;

		using string_t = std::string;
		using boolean_t = bool;
		using double_boolean_t = std::pair<bool, bool>;
		using integer_t = uint64_t;
		using real_t = fixed_point_t;
		using identifier_real_t = std::pair<std::string, real_t>;
		using condition_list_t = std::vector<ConditionNode>;
		using value_t = std::variant<string_t, boolean_t, double_boolean_t, integer_t, real_t, identifier_real_t, condition_list_t>;

	private:
		Condition const* PROPERTY(condition);
		value_t PROPERTY(value);
		HasIdentifier const* PROPERTY(condition_key_item);
		HasIdentifier const* PROPERTY(condition_value_item);
		bool PROPERTY_CUSTOM_PREFIX(valid, is);

		ConditionNode(
			Condition const* new_condition = nullptr, value_t&& new_value = 0,
			bool new_valid = false,
			HasIdentifier const* new_condition_key_item = nullptr,
			HasIdentifier const* new_condition_value_item = nullptr
		);
	};

	struct ConditionManager {
	private:
		CaseInsensitiveIdentifierRegistry<Condition> IDENTIFIER_REGISTRY(condition);
		Condition const* root_condition = nullptr;

		bool add_condition(
			std::string_view identifier, value_type_t value_type, scope_t scope,
			scope_t scope_change = scope_t::NO_SCOPE,
			identifier_type_t key_identifier_type = identifier_type_t::NONE,
			identifier_type_t value_identifier_type = identifier_type_t::NONE
		);

		NodeTools::callback_t<std::string_view> expect_parse_identifier(
			GameManager const& game_manager, identifier_type_t identifier_type,
			NodeTools::callback_t<HasIdentifier const*> callback
		) const;

		NodeTools::node_callback_t expect_condition_node(
			GameManager const& game_manager, Condition const& condition, scope_t this_scope,
			scope_t from_scope, scope_t cur_scope, NodeTools::callback_t<ConditionNode&&> callback
		) const;

		NodeTools::node_callback_t expect_condition_node_list(
			GameManager const& game_manager, scope_t this_scope, scope_t from_scope,
			scope_t cur_scope, bool top_scope, NodeTools::callback_t<ConditionNode&&> callback
		) const;

	public:
		bool setup_conditions(GameManager const& game_manager);

		NodeTools::node_callback_t expect_condition_script(
			GameManager const& game_manager, scope_t initial_scope, scope_t this_scope, scope_t from_scope,
			NodeTools::callback_t<ConditionNode&&> callback
		) const;
	};
}