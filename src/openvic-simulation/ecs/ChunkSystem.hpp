#pragma once

#include <algorithm>
#include <vector>

#include "openvic-simulation/ecs/ChunkView.hpp"
#include "openvic-simulation/ecs/QueryFilter.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"

namespace OpenVic::ecs {

	// CRTP chunk-exec base. Derived class implements:
	//   void tick_chunk(ChunkView<Cs...> view, TickContext const& ctx);
	// and inherits as `: ChunkSystem<DerivedClass, Cs...>`.
	//
	// Useful for tight inner loops over large archetypes — slabs are contiguous, so the
	// inner per-row loop avoids per-element function-call overhead. The decs analogue is
	// `PureSystem`.
	template<typename Derived, typename... Cs>
	struct ChunkSystem {
		// Compile-time access set is computed from Cs... directly (each `C const` becomes
		// Read, `C` becomes Write — same semantics as System<Derived>'s tick-signature
		// inference).
		static constexpr auto declared_access() {
			return std::array<ComponentAccess, sizeof...(Cs)> { ComponentAccess {
				component_type_id_of<std::remove_cvref_t<Cs>>(),
				std::is_const_v<std::remove_reference_t<Cs>> ? AccessMode::Read : AccessMode::Write
			}... };
		}

		static constexpr system_type_id_t type_id() {
			return system_type_id_of<Derived>();
		}

		static constexpr std::array<system_type_id_t, 0> declared_run_after() { return {}; }
		static constexpr std::array<system_type_id_t, 0> declared_run_before() { return {}; }
		static constexpr std::array<component_type_id_t, 0> extra_reads() { return {}; }
		static constexpr std::array<component_type_id_t, 0> extra_writes() { return {}; }
		static constexpr bool is_threaded = false;

		// Sorted-unique component ids defining the iteration query. ChunkSystem doesn't
		// derive from System<>, so it needs its own version — but the result is the same
		// shape: just Cs... folded through component_type_id_of, sorted, deduped.
		// Consumed by the scheduler's query-cache prewarm for multi-system stages.
		static std::vector<component_type_id_t> compute_tick_query_require_ids() {
			std::vector<component_type_id_t> ids = {
				component_type_id_of<std::remove_cvref_t<Cs>>()...
			};
			std::sort(ids.begin(), ids.end());
			ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
			return ids;
		}

		// Sorted-unique exclude ids from the optional `Filters` alias (empty when absent). Mirrors
		// System<>::compute_tick_query_exclude_ids so detail::build_tick_query works uniformly for
		// both bases and the scheduler prewarm sees the same (require, exclude) pair this dispatches.
		static std::vector<component_type_id_t> compute_tick_query_exclude_ids() {
			return system_filters_t<Derived>::exclude_ids();
		}

		void tick_all(World& world, TickContext const& ctx) {
			Derived& self = static_cast<Derived&>(*this);
			Query const q = detail::build_tick_query<Derived>();
			world.template for_each_chunk<std::remove_cvref_t<Cs>...>(q,
				[&](ChunkView<std::remove_cvref_t<Cs>...> view) {
					self.tick_chunk(view, ctx);
				});
		}
	};
}
