#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <span>
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

	// Type-erased holder for ONE contiguous block of `count` component values queued by a
	// bulk create — the batch analogue of PayloadSlot (one allocation per column per batch
	// instead of one per component per entity). Move-only. Tag columns (vtable->size == 0)
	// keep data == nullptr.
	//
	// Ownership protocol mirrors PayloadSlot: the destructor destroys any still-owned values
	// (via ColumnVTable::destroy_n) and frees the block — that covers the un-applied paths
	// (clear(), dropped buffer). apply() instead transfers the values out via
	// World::finalize_reserved_entities_bulk (whose move_construct_n destroys the sources)
	// and then frees the raw memory through release_data() — at that point no constructed
	// values remain, so destroy_n must NOT run again.
	struct PayloadColumn {
		void* data = nullptr;
		ColumnVTable const* vtable = nullptr;
		uint32_t count = 0;

		PayloadColumn() = default;

		PayloadColumn(PayloadColumn const&) = delete;
		PayloadColumn& operator=(PayloadColumn const&) = delete;

		PayloadColumn(PayloadColumn&& other) noexcept
			: data { other.data }, vtable { other.vtable }, count { other.count } {
			other.data = nullptr;
			other.vtable = nullptr;
			other.count = 0;
		}
		PayloadColumn& operator=(PayloadColumn&& other) noexcept {
			if (this != &other) {
				reset();
				data = other.data;
				vtable = other.vtable;
				count = other.count;
				other.data = nullptr;
				other.vtable = nullptr;
				other.count = 0;
			}
			return *this;
		}

		~PayloadColumn() {
			reset();
		}

		void reset() {
			if (data != nullptr && vtable != nullptr && vtable->size > 0) {
				vtable->destroy_n(data, count);
				::operator delete(data, std::align_val_t { vtable->align });
			}
			data = nullptr;
			vtable = nullptr;
			count = 0;
		}

		// Allocate aligned storage for `n` values of vt, but construct nothing — the caller
		// placement-news each element into the block. For tag types (size == 0) data stays
		// nullptr.
		void allocate(ColumnVTable const* vt, uint32_t n) {
			vtable = vt;
			count = n;
			if (vt != nullptr && vt->size > 0 && n > 0) {
				data = ::operator new(vt->size * n, std::align_val_t { vt->align });
			}
		}

		// Release ownership of the block WITHOUT destroying elements (they must already be
		// moved-from / destroyed). Caller frees the returned memory.
		void* release_data() {
			void* d = data;
			data = nullptr;
			vtable = nullptr;
			count = 0;
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

		// Deferred analogue of World::create_immutable_entity. Records a CreateEntity op flagged
		// immutable; at apply() time the finalised entity's slot is stamped immutable. Returns an
		// ImmutableEntityID (a deferred placeholder in parallel mode, a real reserved handle in
		// serial mode), so it cannot be passed to add_component / remove_component on this buffer.
		template<typename... Cs>
		ImmutableEntityID create_immutable_entity(World& world, Cs&&... values);

		// === Bulk entity creation (ECS_SIM_ARCHITECTURE §9 item 4) ===
		// Deferred analogue of World::create_entities: records ONE batch op whose payload is
		// a single contiguous block per non-tag component column (PayloadColumn) instead of
		// count × components PayloadSlots. Same input contract as the World API: one span per
		// non-empty component (pack order, length == count; tags take no span) or no spans to
		// default-construct; input spans are MOVED-FROM at record time. The handles written
		// to `out_ids` (length must equal count) follow the single-create rules per mode:
		//
		// - Serial mode: count real slots reserved up-front in creation order — usable for
		//   same-buffer add_component / destroy_entity, exactly like single creates.
		// - Parallel mode: count SEQUENTIAL deferred placeholders
		//   { deferred_count_ + i, DEFERRED_GENERATION_BIT }; real slots are assigned at
		//   apply() in record order (so a bulk create yields the identical id assignment as
		//   the equivalent create_entity loop). Placeholders in out_ids are never rewritten —
		//   same contract as the single create's returned placeholder.
		//
		// count == 0 records nothing. Returns false (error log, nothing recorded) on
		// out_ids / span length mismatch.
		template<typename... Cs, typename... Spans>
		bool create_entities(
			World& world, std::size_t count, std::span<EntityID> out_ids, Spans&&... spans
		);

		// Bulk analogue of create_immutable_entity: apply() stamps every created entity's
		// slot immutable; out_ids receives ImmutableEntityID handles.
		template<typename... Cs, typename... Spans>
		bool create_immutable_entities(
			World& world, std::size_t count, std::span<ImmutableEntityID> out_ids, Spans&&... spans
		);

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
		// once per system per stage, in the stage's deterministic emit order (ascending
		// system_type_id_t within the stage, independent of registration order). For
		// SystemThreaded, each chunk has its own CommandBuffer; the system-level pending
		// CommandBuffer is built by `merge_from`-ing each per-chunk buffer in chunk_idx
		// ascending order before `apply()` is called.
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
		// Shared body of create_entity / create_immutable_entity — records a CreateEntity op,
		// stamping CreatePayload::immutable. Returns the (real or deferred) raw EntityID.
		template<typename... Cs>
		EntityID record_create_entity(bool immutable, World& world, Cs&&... values);

		// Shared body of create_entities / create_immutable_entities. OutIdT is EntityID or
		// ImmutableEntityID.
		template<typename OutIdT, typename... Cs, typename... Spans>
		bool record_create_entities(
			bool immutable, World& world, std::size_t count, std::span<OutIdT> out_ids,
			Spans&&... spans
		);

		enum class OpKind {
			CreateEntity, // payload: full archetype signature + per-component slots
			CreateEntitiesBulk, // payload: boxed BulkCreatePayload (op.bulk)
			DestroyEntity, // no payload
			AddComponent, // payload: 1 component slot (tag-aware)
			RemoveComponent // no payload — only the type id
		};

		struct CreatePayload {
			std::vector<component_type_id_t> sorted_sig;
			std::vector<ColumnVTable const*> sorted_vtables;
			std::vector<PayloadSlot> sorted_values; // length == sorted_sig.size()
			// When true, apply() stamps the finalised entity's slot immutable. Default false keeps
			// every plain create_entity recording mutable.
			bool immutable = false;
		};

		// Payload of one CreateEntitiesBulk op. Boxed behind a unique_ptr on Op so ordinary
		// ops only pay 8 bytes for it — one heap allocation per BATCH, never per entity.
		struct BulkCreatePayload {
			std::vector<component_type_id_t> sorted_sig;
			std::vector<ColumnVTable const*> sorted_vtables;
			// Parallel to sorted_sig; tag columns hold data == nullptr.
			std::vector<PayloadColumn> columns;
			// Serial mode only: the count real slots reserved at record time, in creation
			// order. Empty in parallel mode (op.eid carries the base placeholder instead;
			// the batch spans placeholders [op.eid.index, op.eid.index + count)).
			std::vector<EntityID> reserved_ids;
			uint32_t count = 0;
			// When true, apply() stamps every finalised entity's slot immutable.
			bool immutable = false;
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
			std::unique_ptr<BulkCreatePayload> bulk; // CreateEntitiesBulk only
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
	EntityID CommandBuffer::record_create_entity(bool immutable, World& world, Cs&&... values) {
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
		op.create.immutable = immutable;
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

	template<typename... Cs>
	EntityID CommandBuffer::create_entity(World& world, Cs&&... values) {
		return record_create_entity<Cs...>(false, world, std::forward<Cs>(values)...);
	}

	template<typename... Cs>
	ImmutableEntityID CommandBuffer::create_immutable_entity(World& world, Cs&&... values) {
		EntityID const eid = record_create_entity<Cs...>(true, world, std::forward<Cs>(values)...);
		return ImmutableEntityID { eid.index, eid.generation };
	}

	template<typename OutIdT, typename... Cs, typename... Spans>
	bool CommandBuffer::record_create_entities(
		bool immutable, World& world, std::size_t count, std::span<OutIdT> out_ids, Spans&&... spans
	) {
		static_assert(sizeof...(Cs) > 0, "CommandBuffer::create_entities requires at least one component");
		static_assert(
			(std::is_same_v<Cs, std::remove_cvref_t<Cs>> && ...),
			"create_entities component types must be plain types (no const/volatile/reference)"
		);
		constexpr std::size_t const non_empty = detail::non_empty_component_count<Cs...>();
		static_assert(
			sizeof...(Spans) == non_empty || sizeof...(Spans) == 0,
			"create_entities takes one span per non-empty component (matched to the non-empty "
			"Cs... in pack order; tags take no span), or no spans to default-construct"
		);
		constexpr bool const use_spans = sizeof...(Spans) > 0;

		detail::BulkSpanTuple<Cs...> typed_spans;
		if constexpr (use_spans) {
			typed_spans = detail::BulkSpanTuple<Cs...> { std::span(std::forward<Spans>(spans))... };
		}

		// Validation routes through the World's out-of-line helper (CommandBuffer is a
		// friend) so this header stays free of the Logger include.
		if constexpr (use_spans) {
			std::size_t span_sizes[non_empty];
			std::size_t si = 0;
			std::apply(
				[&](auto const&... s) {
					((span_sizes[si++] = s.size()), ...);
				},
				typed_spans
			);
			if (!world.bulk_create_sizes_ok_(
				count, out_ids.size(), span_sizes, non_empty, "CommandBuffer::create_entities"
			)) {
				return false;
			}
		} else {
			if (!world.bulk_create_sizes_ok_(
				count, out_ids.size(), nullptr, 0, "CommandBuffer::create_entities"
			)) {
				return false;
			}
		}

		if (count == 0) {
			return true; // record nothing — loop-equivalent
		}

		// Sorted signature + parallel vtable list, paid once per batch.
		component_type_id_t const raw_ids[] = { component_type_id_of<Cs>()... };
		ColumnVTable const* const raw_vtables[] = { &column_vtable_for<Cs>()... };
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

		Op op;
		op.kind = OpKind::CreateEntitiesBulk;
		op.bulk = std::make_unique<BulkCreatePayload>();
		BulkCreatePayload& bulk = *op.bulk;
		bulk.count = static_cast<uint32_t>(count);
		bulk.immutable = immutable;
		bulk.sorted_sig.assign(sorted_ids, sorted_ids + N);
		bulk.sorted_vtables.assign(sorted_vtables, sorted_vtables + N);
		bulk.columns.resize(N);
		for (std::size_t i = 0; i < N; ++i) {
			bulk.columns[i].allocate(sorted_vtables[i], bulk.count);
		}

		// Stage the values: per non-empty component, move-construct (or default-construct)
		// `count` elements from its input span into the column's contiguous block — typed
		// loop, no vtable dispatch.
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			constexpr std::array<std::size_t, N> span_map = detail::bulk_span_index_map<Cs...>();
			auto stage_column = [&]<std::size_t I>() {
				using TC = std::tuple_element_t<I, std::tuple<Cs...>>;
				if constexpr (!std::is_empty_v<TC>) {
					component_type_id_t const id = component_type_id_of<TC>();
					std::size_t target = N;
					for (std::size_t i = 0; i < N; ++i) {
						if (bulk.sorted_sig[i] == id) {
							target = i;
							break;
						}
					}
					TC* const block = static_cast<TC*>(bulk.columns[target].data);
					if constexpr (use_spans) {
						std::span<TC> const src = std::get<span_map[I]>(typed_spans);
						for (std::size_t k = 0; k < count; ++k) {
							::new (block + k) TC(std::move(src[k]));
						}
					} else {
						for (std::size_t k = 0; k < count; ++k) {
							::new (block + k) TC {};
						}
					}
				}
			};
			(stage_column.template operator()<Is>(), ...);
		}(std::index_sequence_for<Cs...> {});

		// Hand out ids. Same per-mode rules as the single create (CommandBuffer.hpp top):
		// serial reserves real slots NOW in creation order; parallel hands out `count`
		// SEQUENTIAL placeholders and bumps deferred_count_ by exactly `count` — merge_from's
		// uniform `index += base` rebase relies on that invariant to shift the whole batch
		// range while preserving intra-range offsets.
		if (parallel_mode_) {
			uint32_t const base = deferred_count_;
			op.eid = EntityID { base, DEFERRED_GENERATION_BIT };
			for (std::size_t i = 0; i < count; ++i) {
				out_ids[i] = OutIdT { base + static_cast<uint32_t>(i), DEFERRED_GENERATION_BIT };
			}
			deferred_count_ += bulk.count;
		} else {
			op.eid = INVALID_ENTITY_ID;
			bulk.reserved_ids.reserve(count);
			for (std::size_t i = 0; i < count; ++i) {
				EntityID const eid = world.reserve_entity_slot();
				bulk.reserved_ids.push_back(eid);
				out_ids[i] = OutIdT { eid.index, eid.generation };
			}
		}

		ops.push_back(std::move(op));
		return true;
	}

	template<typename... Cs, typename... Spans>
	bool CommandBuffer::create_entities(
		World& world, std::size_t count, std::span<EntityID> out_ids, Spans&&... spans
	) {
		return record_create_entities<EntityID, Cs...>(
			false, world, count, out_ids, std::forward<Spans>(spans)...
		);
	}

	template<typename... Cs, typename... Spans>
	bool CommandBuffer::create_immutable_entities(
		World& world, std::size_t count, std::span<ImmutableEntityID> out_ids, Spans&&... spans
	) {
		return record_create_entities<ImmutableEntityID, Cs...>(
			true, world, count, out_ids, std::forward<Spans>(spans)...
		);
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
