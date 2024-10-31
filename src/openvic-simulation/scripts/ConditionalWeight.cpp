#include "ConditionalWeight.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

ConditionalWeight::ConditionalWeight(scope_type_t new_initial_scope, scope_type_t new_this_scope, scope_type_t new_from_scope)
  : initial_scope { new_initial_scope }, this_scope { new_this_scope }, from_scope { new_from_scope } {}

template<typename T>
static NodeCallback auto expect_modifier(
	std::vector<T>& items, scope_type_t initial_scope, scope_type_t this_scope, scope_type_t from_scope
) {
	return [&items, initial_scope, this_scope, from_scope](ast::NodeCPtr node) -> bool {
		fixed_point_t weight = 0;
		bool successful = false;
		bool ret = expect_key("factor", expect_fixed_point(assign_variable_callback(weight)), &successful)(node);
		if (!successful) {
			Logger::info("ConditionalWeight modifier missing factor key!");
			return false;
		}
		ConditionScript condition { initial_scope, this_scope, from_scope };
		ret &= condition.expect_script()(node);
		items.emplace_back(std::make_pair(weight, std::move(condition)));
		return ret;
	};
}

node_callback_t ConditionalWeight::expect_conditional_weight(base_key_t base_key) {
	key_map_t key_map;
	bool successfully_set_up_base_keys = true;

	switch (base_key) {
	case BASE:
		{
			successfully_set_up_base_keys &= add_key_map_entry(
				key_map,
				"base", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(base))
			);

			break;
		}
	case FACTOR:
		{
			successfully_set_up_base_keys &= add_key_map_entry(
				key_map,
				"factor", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(base))
			);

			break;
		}
	case TIME:
		{
			const auto time_callback = [this](std::string_view key, Timespan (*to_timespan)(Timespan::day_t)) -> auto {
				return [this, key, to_timespan](uint32_t value) -> bool {
					if (base == fixed_point_t::_0()) {
						base = fixed_point_t::parse((*to_timespan)(value).to_int());
						return true;
					} else {
						Logger::error(
							"ConditionalWeight cannot have multiple base values - trying to set base to ", value, " ", key,
							" when it already has a value equivalent to ", base, " days!"
						);
						return false;
					}
				};
			};

			successfully_set_up_base_keys &= add_key_map_entries(
				key_map,
				"days", ZERO_OR_ONE, expect_uint<uint32_t>(time_callback("days", Timespan::from_days)),
				"months", ZERO_OR_ONE, expect_uint<uint32_t>(time_callback("months", Timespan::from_months)),
				"years", ZERO_OR_ONE, expect_uint<uint32_t>(time_callback("years", Timespan::from_years))
			);

			break;
		}
	default:
		{
			successfully_set_up_base_keys = false;
		}
	}

	if (!successfully_set_up_base_keys) {
		return [base_key](ast::NodeCPtr node) -> bool {
			Logger::error(
				"Failed to set up base keys for ConditionalWeight with base value: ", static_cast<uint32_t>(base_key)
			);
			return false;
		};
	}

	return expect_dictionary_key_map(
		std::move(key_map),
		"modifier", ZERO_OR_MORE, expect_modifier(condition_weight_items, initial_scope, this_scope, from_scope),
		"group", ZERO_OR_MORE, [this](ast::NodeCPtr node) -> bool {
			condition_weight_group_t items;

			const bool ret = expect_dictionary_keys(
				"modifier", ONE_OR_MORE, expect_modifier(items, initial_scope, this_scope, from_scope)
			)(node);

			if (!items.empty()) {
				condition_weight_items.emplace_back(std::move(items));
				return ret;
			} else {
				Logger::error("ConditionalWeight group must have at least one modifier!");
				return false;
			}
		}
	);
}

struct ConditionalWeight::parse_scripts_visitor_t {
	DefinitionManager const& definition_manager;

	bool operator()(condition_weight_t& condition_weight) const {
		return condition_weight.second.parse_script(false, definition_manager);
	}
	bool operator()(condition_weight_item_t& item) const {
		return std::visit(*this, item);
	}
	template<typename T>
	bool operator()(std::vector<T>& items) const {
		bool ret = true;
		for (T& item : items) {
			ret &= (*this)(item);
		}
		return ret;
	}
};

bool ConditionalWeight::parse_scripts(DefinitionManager const& definition_manager) {
	return parse_scripts_visitor_t { definition_manager }(condition_weight_items);
}
