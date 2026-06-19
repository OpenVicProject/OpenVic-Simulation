#pragma once

#include <cstdint>

#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

namespace OpenVic::ecs {

	// Soft component-pointer that survives across structural mutations of the world by
	// re-resolving on a per-column version mismatch. Cheaper than calling
	// `World::get_component<C>` every time — the fast path is one comparison and an
	// indirection. Storage is the entity ID plus a cached version stamp and pointer.
	//
	// `get(world)` returns the latest pointer (refreshing the cache if stale or the entity
	// has changed archetype), or nullptr if the entity is dead or no longer carries C.
	//
	// Lifetime: a CachedRef may be stored across ticks. It's safe to copy. Holding one
	// after `World::clear_systems` / `end_game_session` is fine but get() may return
	// nullptr once the entity has been swept.
	template<typename C>
	struct CachedRef {
		EntityID entity_id = INVALID_ENTITY_ID;
		uint64_t cached_version = 0;
		C* cached_pointer = nullptr;

		static CachedRef from(World& world, EntityID id) {
			CachedRef ref;
			ref.entity_id = id;
			ref.refresh(world);
			return ref;
		}

		EntityID entity() const {
			return entity_id;
		}

		bool is_valid(World const& world) const {
			return cached_pointer != nullptr && world.is_alive(entity_id);
		}

		void invalidate() {
			cached_pointer = nullptr;
			cached_version = 0;
		}

		// Returns the current component pointer, refreshing the cache if the column has
		// mutated since the last successful resolve or the entity is in a different
		// archetype now. Returns nullptr if the entity is dead or no longer has C.
		C* get(World& world) {
			uint64_t const live_version = world.template component_version_in<C>(entity_id);
			if (live_version == 0) {
				// Entity dead, or doesn't carry C in its current archetype.
				cached_pointer = nullptr;
				cached_version = 0;
				return nullptr;
			}
			if (live_version != cached_version || cached_pointer == nullptr) {
				cached_pointer = world.template get_component<C>(entity_id);
				cached_version = cached_pointer != nullptr ? live_version : 0;
			}
			return cached_pointer;
		}

	private:
		void refresh(World& world) {
			cached_pointer = world.template get_component<C>(entity_id);
			cached_version = cached_pointer != nullptr ? world.template component_version_in<C>(entity_id) : 0;
		}
	};
}
