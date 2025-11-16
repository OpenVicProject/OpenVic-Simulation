#pragma once

#include <ostream>
#include <span>
#include <string_view>
#include <variant>

#include "openvic-simulation/types/EnumBitfield.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct ConditionManager;
	struct ConditionScript;
	struct DefinitionManager;

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

	// Order matters in this enum, for the fallback system to work
	// smaller entities must have smaller integers associated!
	enum class scope_type_t : uint8_t { //TODO: maybe distinguish TRANSPARENT from NO_SCOPE
		NO_SCOPE    = 0,
		POP         = 1 << 0,
		PROVINCE    = 1 << 1,
		STATE       = 1 << 2,
		COUNTRY     = 1 << 3,
		THIS        = 1 << 4,
		FROM        = 1 << 5,
		MAX_SCOPE   = (1 << 6) - 1
	};

	enum class identifier_type_t : uint32_t {
		NO_IDENTIFIER   = 0,
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
		PARTY_POLICY    = 1 << 10,
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
		COUNTRY_EVENT_MODIFIER = 1 << 23,
		PROVINCE_EVENT_MODIFIER = 1 << 24,
		NATIONAL_VALUE  = 1 << 25,
		CULTURE_UNION   = 1 << 26, // same as COUNTRY_TAG but also accepts scope this_union
		CONTINENT       = 1 << 27,
		CRIME           = 1 << 28,
		TERRAIN         = 1 << 29
	};

	/* Allows enum types to be used with bitwise operators. */
	template<> struct enable_bitfield<value_type_t> : std::true_type {};
	template<> struct enable_bitfield<scope_type_t> : std::true_type {};
	template<> struct enable_bitfield<identifier_type_t> : std::true_type {};

	/* Returns true if the values have any bit in common. */
	inline constexpr bool share_value_type(value_type_t lhs, value_type_t rhs) {
		return (lhs & rhs) != value_type_t::NO_TYPE;
	}
	inline constexpr bool share_scope_type(scope_type_t lhs, scope_type_t rhs) {
		return (lhs & rhs) != scope_type_t::NO_SCOPE;
	}
	inline constexpr bool share_identifier_type(identifier_type_t lhs, identifier_type_t rhs) {
		return (lhs & rhs) != identifier_type_t::NO_IDENTIFIER;
	}

#define _BUILD_STRING(entry, share) \
	if (share(value, entry)) { \
		if (type_found) { \
			stream << " | "; \
		} else { \
			type_found = true; \
		} \
		stream << #entry; \
	}

#define BUILD_STRING(entry) _BUILD_STRING(entry, share_value_type)

	inline std::ostream& operator<<(std::ostream& stream, value_type_t value) {
		using enum value_type_t;
		if (value == NO_TYPE) {
			return stream << "[NO_TYPE]";
		}
		bool type_found = false;
		stream << '[';
		BUILD_STRING(IDENTIFIER);
		BUILD_STRING(STRING);
		BUILD_STRING(BOOLEAN);
		BUILD_STRING(BOOLEAN_INT);
		BUILD_STRING(INTEGER);
		BUILD_STRING(REAL);
		BUILD_STRING(COMPLEX);
		BUILD_STRING(GROUP);
		if (!type_found) {
			stream << "INVALID VALUE TYPE";
		}
		return stream << ']';
	}

#undef BUILD_STRING
#define BUILD_STRING(entry) _BUILD_STRING(entry, share_scope_type)

	inline std::ostream& operator<<(std::ostream& stream, scope_type_t value) {
		using enum scope_type_t;
		if (value == NO_SCOPE) {
			return stream << "[NO_SCOPE]";
		}
		bool type_found = false;
		stream << '[';
		BUILD_STRING(COUNTRY);
		BUILD_STRING(STATE);
		BUILD_STRING(PROVINCE);
		BUILD_STRING(POP);
		BUILD_STRING(THIS);
		BUILD_STRING(FROM);
		if (!type_found) {
			stream << "INVALID SCOPE";
		}
		return stream << ']';
	}

#undef BUILD_STRING
#define BUILD_STRING(entry) _BUILD_STRING(entry, share_identifier_type)

	inline std::ostream& operator<<(std::ostream& stream, identifier_type_t value) {
		using enum identifier_type_t;
		if (value == NO_IDENTIFIER) {
			return stream << "[NO_IDENTIFIER]";
		}
		bool type_found = false;
		stream << '[';
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
		BUILD_STRING(PARTY_POLICY);
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
		BUILD_STRING(COUNTRY_EVENT_MODIFIER);
		BUILD_STRING(PROVINCE_EVENT_MODIFIER);
		BUILD_STRING(NATIONAL_VALUE);
		BUILD_STRING(CULTURE_UNION);
		BUILD_STRING(CONTINENT);
		BUILD_STRING(CRIME);
		BUILD_STRING(TERRAIN);
		if (!type_found) {
			stream << "INVALID IDENTIFIER TYPE";
		}
		return stream << ']';
	}

#undef BUILD_STRING
#undef _BUILD_STRING

	struct Condition : HasIdentifier {
		friend struct ConditionManager;
		using enum identifier_type_t;

	public:
		const value_type_t value_type;
		const scope_type_t scope;
		const scope_type_t scope_change;
		const identifier_type_t key_identifier_type;
		const identifier_type_t value_identifier_type;

		Condition(
			std::string_view new_identifier, value_type_t new_value_type, scope_type_t new_scope,
			scope_type_t new_scope_change, identifier_type_t new_key_identifier_type,
			identifier_type_t new_value_identifier_type
		);
		Condition(Condition&&) = default;
	};

	struct ConditionNode {
		friend struct ConditionManager;
		friend struct ConditionScript;

		using string_t = memory::string;
		using boolean_t = bool;
		using double_boolean_t = std::pair<bool, bool>;
		using integer_t = uint64_t;
		using real_t = fixed_point_t;
		using identifier_real_t = std::pair<memory::string, real_t>;
		using condition_list_t = memory::vector<ConditionNode>;
		using value_t = std::variant<
			string_t, boolean_t, double_boolean_t, integer_t, real_t, identifier_real_t, condition_list_t
		>;

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
			scope_type_t from_scope, NodeTools::callback_t<ConditionNode&&> callback, std::span<const ast::NodeCPtr> nodes
		) const;
	};
}
