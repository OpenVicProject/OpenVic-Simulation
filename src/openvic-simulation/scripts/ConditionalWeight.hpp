#pragma once

#include <cstdint>
#include <utility>
#include <variant>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/core/object/FixedPoint.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/scripts/Condition.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"

namespace OpenVic {
	enum class conditional_weight_type_t : uint8_t {
		BASE, FACTOR_ADD, FACTOR_MUL, TIME
	};

	constexpr bool conditional_weight_type_is_additive(conditional_weight_type_t type) {
		return type == conditional_weight_type_t::BASE || type == conditional_weight_type_t::FACTOR_ADD;
	}

	constexpr bool conditional_weight_type_is_multiplicative(conditional_weight_type_t type) {
		return type == conditional_weight_type_t::FACTOR_MUL || type == conditional_weight_type_t::TIME;
	}

	using condition_weight_t = std::pair<fixed_point_t, ConditionScript>;
	using condition_weight_group_t = memory::vector<condition_weight_t>;
	using condition_weight_item_t = std::variant<condition_weight_t, condition_weight_group_t>;

	template<conditional_weight_type_t TYPE>
	struct ConditionalWeight {

	private:
		fixed_point_t PROPERTY(base, 0);
		memory::vector<condition_weight_item_t> PROPERTY(condition_weight_items);
		scope_type_t PROPERTY(initial_scope);
		scope_type_t PROPERTY(this_scope);
		scope_type_t PROPERTY(from_scope);

	public:
		ConditionalWeight(
			scope_type_t new_initial_scope = scope_type_t::NO_SCOPE,
			scope_type_t new_this_scope = scope_type_t::NO_SCOPE,
			scope_type_t new_from_scope = scope_type_t::NO_SCOPE
		);
		ConditionalWeight(ConditionalWeight&&) = default;
		ConditionalWeight& operator=(ConditionalWeight&&) = default;

		NodeTools::node_callback_t expect_conditional_weight();

		bool parse_scripts(DefinitionManager const& definition_manager);

		// Used mainly to check if a ConditionalWeight has been properly initialised by comparing against {}
		bool operator==(ConditionalWeight const& other) const;
	};

	using ConditionalWeightBase = ConditionalWeight<conditional_weight_type_t::BASE>;
	using ConditionalWeightFactorAdd = ConditionalWeight<conditional_weight_type_t::FACTOR_ADD>;
	using ConditionalWeightFactorMul = ConditionalWeight<conditional_weight_type_t::FACTOR_MUL>;
	using ConditionalWeightTime = ConditionalWeight<conditional_weight_type_t::TIME>;
}
