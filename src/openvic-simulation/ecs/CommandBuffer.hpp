#pragma once

#include <algorithm>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

#include "openvic-simulation/ecs/Archetype.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

namespace OpenVic::ecs {

	// Type-erased holder for one component value queued in a CommandBuffer. Owns the
	// allocation; destructor cleans up correctly regardless of payload type. Move-only.
	struct PayloadSlot {
		void* data = nullptr;
		ColumnVTable const* vtable = nullptr;

		PayloadSlot() = default;

		PayloadSlot(PayloadSlot const&) = delete;
		PayloadSlot& operator=(PayloadSlot const&) = delete;

		PayloadSlot(PayloadSlot&& other) noexcept : data { other.data }, vtable { other.vtable } {
			other.data = nullptr;
			other.vtable = nullptr;
		}
		PayloadSlot& operator=(PayloadSlot&& other) noexcept {
			if (this != &other) {
				reset();
				data = other.data;
				vtable = other.vtable;
				other.data = nullptr;
				other.vtable = nullptr;
			}
			return *this;
		}

		~PayloadSlot() {
			reset();
		}

		void reset() {
			if (data != nullptr && vtable != nullptr && vtable->size > 0) {
				vtable->destroy(data);
				::operator delete(data, std::align_val_t { vtable->align });
			}
			data = nullptr;
			vtable = nullptr;
		}

		// Allocate aligned storage for one value of vt, but do not construct anything.
		// Caller placement-news into `data`. For tag types (size == 0) data stays nullptr.
		void allocate(ColumnVTable const* vt) {
			vtable = vt;
			if (vt != nullptr && vt->size > 0) {
				data = ::operator new(vt->size, std::align_val_t { vt->align });
			}
		}

		// Release ownership of the payload (caller must destroy the value or move it
		// onwards). Used during apply() — the archetype's column move-constructs from
		// `data`, then we still need to free the now-moved-from allocation.
		void* release_data() {
			void* d = data;
			data = nullptr;
			vtable = nullptr;
			return d;
		}
	};

	struct CommandBuffer {
		// In **serial mode** (default): reserves a slot in `world` synchronously and returns its
		// real EntityID. `world.is_alive(eid)` returns false until `apply()` finalises it.
		// Components are copied / moved into a type-erased PayloadSlot per component.
		//
		// In **parallel mode** (`set_parallel_mode(true)` — set by SystemThreaded on every
		// per-chunk buffer): no World mutation. Returns a *deferred placeholder* EntityID
		// `{ index = local_seq, generation = DEFERRED_GENERATION_BIT }`. The placeholder
		// satisfies `is_valid()` and `is_deferred()`, fails `world.is_alive()`, and is accepted
		// by other ops on the same buffer (`add_component`, `destroy_entity`, `remove_component`).
		// `apply()` resolves placeholders to real EntityIDs at the stage barrier, on a single
		// thread, in record order. Allocation order is therefore worker-count-invariant given
		// the chunk_idx-ascending merge done by `SystemThreaded::tick_all` before apply.
		template<typename... Cs>
		EntityID create_entity(World& world, Cs&&... values);

		inline void destroy_entity(EntityID id) {
			Op op;
			op.kind = OpKind::DestroyEntity;
			op.eid = id;
			ops.push_back(std::move(op));
		}

		template<typename C>
		void add_component(EntityID id, C&& value);

		template<typename C>
		void add_component(EntityID id); // default-construct

		template<typename C>
		void remove_component(EntityID id);

		// Drains the buffer onto the world in record order. The scheduler invokes this
		// once per system per stage in registration_index order. For SystemThreaded, each
		// chunk has its own CommandBuffer; the system-level pending CommandBuffer is built
		// by `merge_from`-ing each per-chunk buffer in chunk_idx ascending order before
		// `apply()` is called.
		void apply(World& world);

		// Splice `other`'s queued ops onto the end of our op vector. After return, `other`
		// is empty (op_count() == 0). Used by `SystemThreaded::tick_all` to combine the
		// per-chunk buffers into the system's pending buffer in chunk_idx ascending order.
		// PayloadSlot moves are zero-copy (just pointer transfer).
		void merge_from(CommandBuffer&& other);

		// Resets without applying — every queued payload is destroyed via its vtable. After
		// clear(), op_count() == 0 and empty() == true.
		void clear();

		// When set, `create_entity` switches to deferred mode: no World mutation, returns a
		// placeholder EntityID resolved at apply time. add_component / remove_component /
		// destroy_entity continue to record op intent (they always defer). Set by
		// SystemThreaded on every per-chunk buffer; cleared on the system's pending buffer
		// before merge_from + apply so the resolution path is exercised on a single thread.
		// Default false.
		void set_parallel_mode(bool enabled) {
			parallel_mode_ = enabled;
		}
		bool parallel_mode() const {
			return parallel_mode_;
		}

		// Number of deferred CreateEntity ops queued (placeholder entities pending resolution).
		// Bumped by `create_entity` while parallel_mode is set, summed across `merge_from` calls,
		// reset to 0 by `apply` and `clear`. Used by `apply` to size the placeholder→real map.
		uint32_t deferred_count() const {
			return deferred_count_;
		}

		std::size_t op_count() const {
			return ops.size();
		}
		bool empty() const {
			return ops.empty();
		}

	private:
		enum class OpKind {
			CreateEntity, // payload: full archetype signature + per-component slots
			DestroyEntity, // no payload
			AddComponent, // payload: 1 component slot (tag-aware)
			RemoveComponent // no payload — only the type id
		};

		struct CreatePayload {
			std::vector<component_type_id_t> sorted_sig;
			std::vector<ColumnVTable const*> sorted_vtables;
			std::vector<PayloadSlot> sorted_values; // length == sorted_sig.size()
		};

		struct AddPayload {
			component_type_id_t id;
			PayloadSlot value; // .vtable always set (even for tag — size==0); .data null for tag/default
			bool is_default; // true when add_component<C>() with no value
		};

		struct Op {
			OpKind kind;
			EntityID eid;
			component_type_id_t remove_id = 0; // RemoveComponent only
			CreatePayload create; // CreateEntity only
			AddPayload add; // AddComponent only
		};

		std::vector<Op> ops;
		bool parallel_mode_ = false;
		// Count of deferred (placeholder) CreateEntity ops queued in this buffer. When two
		// buffers are spliced via `merge_from`, the receiver rebases incoming placeholder
		// `index`es by its current `deferred_count_` so placeholders stay unique post-merge.
		// `apply` consumes this to size its placeholder→real-EntityID resolution map; `clear`
		// and `apply` reset it to 0.
		uint32_t deferred_count_ = 0;
	};

	// === template definitions ===

	template<typename... Cs>
	EntityID CommandBuffer::create_entity(World& world, Cs&&... values) {
		static_assert(sizeof...(Cs) > 0, "CommandBuffer::create_entity requires at least one component");

		// Build the same sorted signature as World::create_entity does, recording the
		// vtable pointer alongside each id.
		component_type_id_t const raw_ids[] = { component_type_id_of<std::remove_cvref_t<Cs>>()... };
		ColumnVTable const* const raw_vtables[] = { &column_vtable_for<std::remove_cvref_t<Cs>>()... };
		constexpr std::size_t const N = sizeof...(Cs);

		component_type_id_t sorted_ids[N];
		ColumnVTable const* sorted_vtables[N];
		for (std::size_t i = 0; i < N; ++i) {
			sorted_ids[i] = raw_ids[i];
			sorted_vtables[i] = raw_vtables[i];
		}
		for (std::size_t i = 0; i < N; ++i) {
			for (std::size_t j = i + 1; j < N; ++j) {
				if (sorted_ids[j] < sorted_ids[i]) {
					std::swap(sorted_ids[i], sorted_ids[j]);
					std::swap(sorted_vtables[i], sorted_vtables[j]);
				}
			}
		}

		// In parallel mode (SystemThreaded per-chunk buffers), defer slot reservation: no World
		// mutation here, just hand back a placeholder EntityID with DEFERRED_GENERATION_BIT set.
		// `apply()` allocates the real slot at the stage barrier and rewrites the placeholder.
		// In serial mode, reserve a real slot up-front so callers get a usable EntityID
		// immediately (e.g. for `cmd.add_component(eid, ...)` later in the same recording).
		EntityID const eid = parallel_mode_
			? EntityID { deferred_count_++, DEFERRED_GENERATION_BIT }
			: world.reserve_entity_slot();

		Op op;
		op.kind = OpKind::CreateEntity;
		op.eid = eid;
		op.create.sorted_sig.assign(sorted_ids, sorted_ids + N);
		op.create.sorted_vtables.assign(sorted_vtables, sorted_vtables + N);
		op.create.sorted_values.resize(N);
		for (std::size_t i = 0; i < N; ++i) {
			op.create.sorted_values[i].allocate(sorted_vtables[i]);
		}

		// Move each value into the corresponding sorted slot. Use a fold expression with the
		// raw (unsorted) parameter pack and look up the sorted index.
		auto place = [&]<typename C>(C&& value) {
			using TC = std::remove_cvref_t<C>;
			component_type_id_t const id = component_type_id_of<TC>();
			std::size_t target = N;
			for (std::size_t i = 0; i < N; ++i) {
				if (op.create.sorted_sig[i] == id) {
					target = i;
					break;
				}
			}
			if constexpr (!std::is_empty_v<TC>) {
				::new (op.create.sorted_values[target].data) TC(std::forward<C>(value));
			} else {
				(void) value;
				(void) target;
			}
		};
		(place(std::forward<Cs>(values)), ...);

		ops.push_back(std::move(op));
		return eid;
	}

	template<typename C>
	void CommandBuffer::add_component(EntityID id, C&& value) {
		using TC = std::remove_cvref_t<C>;
		Op op;
		op.kind = OpKind::AddComponent;
		op.eid = id;
		op.add.id = component_type_id_of<TC>();
		op.add.is_default = false;
		op.add.value.allocate(&column_vtable_for<TC>());
		if constexpr (!std::is_empty_v<TC>) {
			::new (op.add.value.data) TC(std::forward<C>(value));
		} else {
			(void) value;
		}
		ops.push_back(std::move(op));
	}

	template<typename C>
	void CommandBuffer::add_component(EntityID id) {
		using TC = std::remove_cvref_t<C>;
		Op op;
		op.kind = OpKind::AddComponent;
		op.eid = id;
		op.add.id = component_type_id_of<TC>();
		op.add.is_default = true;
		op.add.value.allocate(&column_vtable_for<TC>());
		if constexpr (!std::is_empty_v<TC>) {
			::new (op.add.value.data) TC {};
		}
		ops.push_back(std::move(op));
	}

	template<typename C>
	void CommandBuffer::remove_component(EntityID id) {
		using TC = std::remove_cvref_t<C>;
		Op op;
		op.kind = OpKind::RemoveComponent;
		op.eid = id;
		op.remove_id = component_type_id_of<TC>();
		ops.push_back(std::move(op));
	}
}
