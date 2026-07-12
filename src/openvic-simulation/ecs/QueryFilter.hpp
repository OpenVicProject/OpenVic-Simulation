#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"

namespace OpenVic::ecs {

	// Compile-time archetype-filter vocabulary for systems. A System (or ChunkSystem) opts into
	// extra archetype filtering by declaring a `Filters` member alias:
	//
	//   struct MySystem : System<MySystem> {
	//       using Filters = ecs::Filter<ecs::Without<Frozen>>;
	//       void tick(TickContext const& ctx, Position& pos);
	//   };
	//
	// `Without<C>` makes the system's iteration query SKIP every archetype that carries C. The
	// system's required components still come from its tick parameter pack (System<>) or template
	// list (ChunkSystem<>); `Filters` only adds the exclude set. A system with no `Filters` alias
	// behaves exactly as before (empty exclude list).
	//
	// `Filter` / `Without` feed `Query::exclude_ids` through `compute_tick_query_exclude_ids()` on
	// the system bases. The exclude ids are stored on the SystemRegistration and used IDENTICALLY by
	// the scheduler's query-cache prewarm and by every dispatch path — see detail::build_tick_query.
	// That shared-builder discipline is load-bearing: if the prewarmed key and the dispatch key ever
	// disagreed, a worker thread would mutate the World's `mutable` query_cache concurrently.
	//
	// The shape is intentionally extensible — a future `With<C>` (presence-only require) or
	// `Optional<C>` (nullable data) marker can join `Filter<...>` without changing this surface — but
	// only `Without<C>` is wired today.

	// Exclusion marker: archetypes containing C are not iterated.
	template<typename C>
	struct Without {};

	// True iff F is a Without<...> marker.
	template<typename F>
	struct is_without : std::false_type {};
	template<typename C>
	struct is_without<Without<C>> : std::true_type {};

	// Maps a Without<C> marker to C's component id.
	template<typename F>
	struct without_id;
	template<typename C>
	struct without_id<Without<C>> {
		static component_type_id_t value() {
			return component_type_id_of<C>();
		}
	};

	// A system's filter set. Currently only Without<C> entries are supported; the static_assert
	// turns any other marker into a clear compile error rather than a silent no-op.
	template<typename... Fs>
	struct Filter {
		static_assert(
			(is_without<Fs>::value && ...),
			"ecs::Filter currently supports only ecs::Without<C> entries."
		);

		// Sorted-unique exclude ids. Runtime (not constexpr) to mirror
		// System<>::compute_tick_query_require_ids — both feed Query::*_ids vectors.
		static std::vector<component_type_id_t> exclude_ids() {
			std::vector<component_type_id_t> ids = { without_id<Fs>::value()... };
			std::sort(ids.begin(), ids.end());
			ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
			return ids;
		}
	};

	// Detects a `Derived::Filters` member alias, defaulting to an empty Filter<> when absent. Uses
	// the void_t idiom (not a `requires` clause) — MSVC handles this trait-specialisation form of
	// member-type detection more reliably.
	template<typename T, typename = void>
	struct system_filters {
		using type = Filter<>;
	};
	template<typename T>
	struct system_filters<T, std::void_t<typename T::Filters>> {
		using type = typename T::Filters;
	};
	template<typename T>
	using system_filters_t = typename system_filters<T>::type;
}
