#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"

namespace OpenVic::dataloader::dsl::detail {
	template<typename... Ts>
	struct type_list {};

	template<typename T, typename List>
	inline constexpr bool contains_v = false;
	template<typename T, typename... Ts>
	inline constexpr bool contains_v<T, type_list<Ts...>> = (std::is_same_v<T, Ts> || ...);

	template<typename List, typename T>
	struct append_unique {
		using type = List;
	};
	template<typename... Ts, typename T>
	struct append_unique<type_list<Ts...>, T> {
		using type = std::conditional_t<contains_v<T, type_list<Ts...>>, type_list<Ts...>, type_list<Ts..., T>>;
	};

	template<typename Tuple>
	struct to_list;
	template<typename... Ts>
	struct to_list<std::tuple<Ts...>> {
		using type = type_list<std::decay_t<Ts>...>;
	};

	template<typename Rule>
	struct contrib {
		static constexpr bool is_void = false;
		static constexpr bool has_prepare = false;
		using type = std::tuple<>;
	};

	template<typename Rule>
	requires requires(Rule& r, TraverseContext& c) { r.event_generate_parsers(c); }
	struct contrib<Rule> {
		using R = decltype(std::declval<Rule&>().event_generate_parsers(std::declval<TraverseContext&>()));
		static constexpr bool is_void = std::is_void_v<R>;
		static constexpr bool has_prepare = true;

		using type = std::conditional_t<is_void, std::tuple<>, std::conditional_t<requires {
			                                std::tuple_cat(std::tuple {}, std::declval<R>());
		                                }, R, std::tuple<R>>>;
	};

	// Decide which indices to call
	template<typename Seen, typename RulesTuple, std::size_t I, std::size_t N>
	struct decide;

	template<typename Seen, typename RulesTuple, std::size_t N>
	struct decide<Seen, RulesTuple, N, N> {
		using indices = std::index_sequence<>;
	};

	template<typename Seen, typename RulesTuple, std::size_t I, std::size_t N>
	struct decide {
		using Rule = std::tuple_element_t<I, RulesTuple>;
		using C = contrib<Rule>;
		using contrib_t = typename C::type;
		using CL = typename to_list<contrib_t>::type;

		// new types?
		static constexpr bool has_new = []<typename... Us>(type_list<Us...>) {
			if constexpr (sizeof...(Us) == 0) {
				return false;
			} else {
				return (!contains_v<Us, Seen> || ...);
			}
		}(CL {});

		// Call when:
		//   - the rule has a prepare function AND
		//   - (it returns void  OR  it introduces a new type)
		static constexpr bool call_it = C::has_prepare && (C::is_void || has_new);

		// next Seen (only non-void contributions update Seen)
		template<typename L, typename... Us>
		struct app;
		template<typename L>
		struct app<L> {
			using type = L;
		};
		template<typename L, typename U, typename... Us>
		struct app<L, U, Us...> {
			using type = typename app<typename append_unique<L, U>::type, Us...>::type;
		};
		template<typename... Us>
		static auto next(type_list<Us...>) -> typename app<Seen, Us...>::type;
		using next_seen = std::conditional_t<
		    C::is_void,
		    Seen, // void does not change Seen
		    decltype(next(CL {}))>;

		using rest = decide<next_seen, RulesTuple, I + 1, N>;

		template<std::size_t... Js>
		static auto prepend(std::index_sequence<Js...>) -> std::index_sequence<I, Js...>;

		using indices = std::conditional_t<call_it, decltype(prepend(typename rest::indices {})), typename rest::indices>;
	};

	// Call only the selected indices and concatenate
	template<typename RulesTuple, std::size_t... Is>
	auto call_indices(TraverseContext& ctx, RulesTuple& rules, std::index_sequence<Is...>) {
		auto call = [&]<std::size_t I>() {
			auto& rule = std::get<I>(rules);
			if constexpr (requires { rule.event_generate_parsers(ctx); }) {
				using Ret = decltype(rule.event_generate_parsers(ctx));
				if constexpr (std::is_void_v<Ret>) {
					rule.event_generate_parsers(ctx);
					return std::tuple {};
				} else if constexpr (requires { std::tuple_cat(std::tuple {}, rule.event_generate_parsers(ctx)); }) {
					return rule.event_generate_parsers(ctx);
				} else {
					return std::tuple { rule.event_generate_parsers(ctx) };
				}
			} else {
				return std::tuple {};
			}
		};
		return std::tuple_cat(call.template operator()<Is>()...);
	}

	template<typename RulesTuple>
	auto unique_event_generate_parsers(TraverseContext& ctx, RulesTuple& rules) {
		constexpr std::size_t N = std::tuple_size_v<RulesTuple>;
		using indices = typename decide<type_list<>, RulesTuple, 0, N>::indices;
		return call_indices(ctx, rules, indices {});
	}
}
