#pragma once

#include <cstdint>
#include <functional>
#include <tuple>

namespace OpenVic::ecs {
	using archetype_alias_id_t = uint64_t;

	template<typename ...Cs>
	struct ArchetypeAlias {
		using component_tuple = std::tuple<Cs...>;
	};

	// Helper to hold a component_tuple as a type
	template<typename... Ts>
	struct archetype_component_list {};

	template<typename T>
	struct archetype_component_list_extract;

	template<typename... Ts>
	struct archetype_component_list_extract<std::tuple<Ts...>> {
		using type = archetype_component_list<Ts...>;
	};

	template<template<typename...> class Templ, typename Tuple>
	struct archetype_component_list_apply;

	template<template<typename...> class Templ, typename... Ts>
	struct archetype_component_list_apply<Templ, std::tuple<Ts...>> {
		using type = Templ<Ts...>;
	};

	template<typename Fn, typename A>
	concept archetype_function = requires(Fn fn, A::component_tuple tuple) {
		std::apply(fn, tuple);
	};

	template<typename Fn, typename A, typename... Args>
	concept archetype_args_function = requires(Fn fn, A::component_tuple tuple, Args... args) {
		std::apply(std::bind_front(fn, args...), tuple);
	};

	template<typename Fn, typename A, template <typename...> typename C>
	concept archetype_contained_function = requires(Fn fn, archetype_component_list_apply<C, typename A::component_tuple>::type container) {
		fn(container);
	};
}

// Usage Example:
//   ECS_DEFINE_ARCHETYPE(LeaderArchetype, LeaderTemplate);
#define ECS_DEFINE_ARCHETYPE(Type, ...) \
	struct Type : OpenVic::ecs::ArchetypeAlias<__VA_ARGS__> {};
