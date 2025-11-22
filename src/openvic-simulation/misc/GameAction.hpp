#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/types/UniqueId.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"

namespace OpenVic {
namespace detail {
	template<typename ...Args>
	struct reduce_tuple {
		using type = std::tuple<Args...>;
	};

	template<typename T1, typename T2>
	struct reduce_tuple<T1, T2> {
		using type = std::pair<T1, T2>;
	};

	template<typename T>
	struct reduce_tuple<T> {
		using type = T;
	};

	struct ts_op_structured_binding_passthrough {};
}

// To add a new game action:
// 1. **Copy** an existing line.
// 2. **Edit** the game action name and its argument type.
//
// The format for each action is: X(action_name, argument_type1, argument_type2, ...)
// `none` is handled as example throughout the file
#define FOR_EACH_GAME_ACTION(X) \
X(tick, std::monostate) \
X(set_pause, bool) \
X(set_speed, int64_t) \
X(set_ai, country_index_t, bool) \
X(expand_province_building, province_index_t, building_type_index_t) \
X(set_strata_tax, country_index_t, strata_index_t, fixed_point_t) \
X(set_army_spending, country_index_t, fixed_point_t) \
X(set_navy_spending, country_index_t, fixed_point_t) \
X(set_construction_spending, country_index_t, fixed_point_t) \
X(set_education_spending, country_index_t, fixed_point_t) \
X(set_administration_spending, country_index_t, fixed_point_t) \
X(set_social_spending, country_index_t, fixed_point_t) \
X(set_military_spending, country_index_t, fixed_point_t) \
X(set_tariff_rate, country_index_t, fixed_point_t) \
X(start_research, country_index_t, technology_index_t) \
X(set_good_automated, country_index_t, good_index_t, bool) \
X(set_good_trade_order, country_index_t, good_index_t, bool, fixed_point_t) \
X(create_leader, country_index_t, unit_branch_t) \
X(set_use_leader, unique_id_t, bool) \
X(set_auto_create_leaders, country_index_t, bool) \
X(set_auto_assign_leaders, country_index_t, bool) \
X(set_mobilise, country_index_t, bool)
// <--- ADD NEW GAME ACTIONS HERE (copy/edit an X(...) line)

//the argument type alias for each game action
#define ARG_TYPE(name) name##_argument_t

//define type aliases for game action arguments
//example:
struct none_argument_t : type_safe::strong_typedef<none_argument_t, std::monostate> {
	using strong_typedef::strong_typedef;
	constexpr none_argument_t(std::monostate const& value) : strong_typedef(value) {}
	constexpr none_argument_t(std::monostate&& value) 
		: strong_typedef(static_cast<std::monostate&&>(value)) {} 
};
#define USING_ARG_TYPE(name, ...) \
struct ARG_TYPE(name) : type_safe::strong_typedef<ARG_TYPE(name), detail::reduce_tuple<__VA_ARGS__>::type>, detail::ts_op_structured_binding_passthrough { \
	using strong_typedef::strong_typedef; \
	constexpr ARG_TYPE(name)(detail::reduce_tuple<__VA_ARGS__>::type const& value) : strong_typedef(value) {} \
	constexpr ARG_TYPE(name)(detail::reduce_tuple<__VA_ARGS__>::type && value) \
		: strong_typedef(static_cast<detail::reduce_tuple<__VA_ARGS__>::type&&>(value)) {} \
};

FOR_EACH_GAME_ACTION(USING_ARG_TYPE)
#undef USING_ARG_TYPE

//define game_action_t as a variant of all possible game action arguments
#define COMMA_ARG_TYPE(name, ...) , ARG_TYPE(name)
		using game_action_t = std::variant<
			none_argument_t
			FOR_EACH_GAME_ACTION(COMMA_ARG_TYPE)
		>;
#undef COMMA_ARG_TYPE

	struct InstanceManager;

	struct GameActionManager {
	private:
		struct VariantVisitor {
		private:
			InstanceManager& instance_manager;
		public:
			constexpr VariantVisitor(InstanceManager& new_instance_manager) : instance_manager { new_instance_manager } {}
//define a method for each game method
#define DEFINE_ARG_TYPE_AND_METHOD(name, ...) bool operator()(ARG_TYPE(name) const& argument) const;
//example:
bool operator()(none_argument_t const& argument) const;
	FOR_EACH_GAME_ACTION(DEFINE_ARG_TYPE_AND_METHOD)
#undef DEFINE_ARG_TYPE_AND_METHOD
		};

		VariantVisitor visitor;
	public:
		constexpr GameActionManager(InstanceManager& new_instance_manager) : visitor { new_instance_manager } {}

		constexpr bool execute_game_action(game_action_t const& game_action) const {
			return std::visit(visitor, game_action);
		}
	};

	template<std::size_t I, std::derived_from<OpenVic::detail::ts_op_structured_binding_passthrough> T>
	decltype(auto) get(T& value) {
		return std::get<I>(type_safe::get(value));
	}

	template<std::size_t I, std::derived_from<OpenVic::detail::ts_op_structured_binding_passthrough> T>
	decltype(auto) get(T const& value) {
		return std::get<I>(type_safe::get(value));
	}
}

namespace std {
	template<std::derived_from<OpenVic::detail::ts_op_structured_binding_passthrough> T>
	struct tuple_size<T> : tuple_size<std::decay_t<decltype(type_safe::get(std::declval<T>()))>> {};

	template<std::size_t I, std::derived_from<OpenVic::detail::ts_op_structured_binding_passthrough> T>
	struct tuple_element<I, T> : tuple_element<I, std::decay_t<decltype(type_safe::get(std::declval<T>()))>> {};

	template<std::size_t I, std::derived_from<OpenVic::detail::ts_op_structured_binding_passthrough> T>
	decltype(auto) get(T& value) {
		return OpenVic::get(value);
	}

	template<std::size_t I, std::derived_from<OpenVic::detail::ts_op_structured_binding_passthrough> T>
	decltype(auto) get(T const& value) {
		return OpenVic::get(value);
	}
}

#undef ARG_TYPE
#undef FOR_EACH_GAME_ACTION