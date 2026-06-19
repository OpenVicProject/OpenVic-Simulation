#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

#include "openvic-simulation/ecs/Chunk.hpp"
#include "openvic-simulation/ecs/ChunkPool.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"

namespace OpenVic::ecs {

	// Type-erased operations needed to manage a component column without naming the type.
	// For tag (zero-size, std::is_empty) component types, `size` and `align` are 0 and the
	// move/destroy thunks are no-ops — the archetype then holds no slab for that column,
	// only a row count tracked by chunks.
	struct ColumnVTable {
		std::size_t size;
		std::size_t align;
		void (*move_construct)(void* dst, void* src);
		void (*destroy)(void* dst);
	};

	template<typename C>
	inline ColumnVTable const& column_vtable_for() {
		if constexpr (std::is_empty_v<C>) {
			static ColumnVTable const v {
				0,
				0,
				[](void*, void*) {},
				[](void*) {}
			};
			return v;
		} else {
			static ColumnVTable const v {
				sizeof(C),
				alignof(C),
				[](void* dst, void* src) {
					::new (dst) C(std::move(*static_cast<C*>(src)));
					static_cast<C*>(src)->~C();
				},
				[](void* dst) {
					static_cast<C*>(dst)->~C();
				}
			};
			return v;
		}
	}

	// Sentinel for "tag column has no slab" / "component not in archetype".
	constexpr std::size_t NO_COLUMN_OFFSET = static_cast<std::size_t>(-1);
	constexpr std::size_t NO_COLUMN_INDEX = static_cast<std::size_t>(-1);

	// One archetype = one unique sorted set of component type IDs.
	// Storage is a list of fixed-size 16 KB chunks. Each chunk holds up to `chunk_capacity`
	// rows; rows fill chunks left-to-right. When the last chunk is full, a fresh chunk is
	// allocated. Removal swap-pops with the *last* row of the *last* chunk (cross-chunk swap
	// when needed), and an emptied trailing chunk is dropped.
	struct Archetype {
		std::vector<component_type_id_t> signature;
		std::vector<ColumnVTable const*> vtables;
		// Byte offset of each column's slab within a chunk's `data`. Tag columns
		// (vtable->size == 0) carry NO_COLUMN_OFFSET; their data must never be dereferenced.
		std::vector<std::size_t> column_offsets;
		// Per-column monotonic version counter, bumped on every push / swap-pop / migration
		// touching the column. Replaces the per-Column version stamp from the pre-chunked
		// design — `World::component_version_in<C>(eid)` reads from here.
		std::vector<uint64_t> column_versions;
		std::size_t chunk_capacity = 0; // rows per chunk; constant for the archetype's life
		std::size_t total_entity_count = 0;
		std::vector<DataChunk> chunks;
		// Bitfield prefilter: one bit per component, derived from `id % 63`. Used by
		// `World::resolve_query_cache` to fast-reject archetypes before the sorted-set walk.
		uint64_t matcher_hash = 0;
		// Non-owning pointer to the World's ChunkPool. Set by World when the archetype is
		// created. Nullable: tests that construct an Archetype outside a World fall through
		// to ::operator new / ::operator delete in allocate_chunk and the destructor.
		ChunkPool* chunk_pool = nullptr;

		Archetype() = default;
		Archetype(Archetype const&) = delete;
		Archetype& operator=(Archetype const&) = delete;
		Archetype(Archetype&&) = default;
		Archetype& operator=(Archetype&&) = default;

		// Destroy every live component and release each chunk's backing block. With a
		// ChunkPool wired in, World::~World calls drain_to_pool first and the loop below
		// then sees an empty `chunks` vector — this destructor is the non-pool fallback
		// (bare Archetype in tests, or any path that constructs an Archetype directly).
		~Archetype() {
			for (std::size_t ci = 0; ci < chunks.size(); ++ci) {
				DataChunk& chunk = chunks[ci];
				for (std::size_t row = 0; row < chunk.count; ++row) {
					for (std::size_t col = 0; col < signature.size(); ++col) {
						if (column_offsets[col] == NO_COLUMN_OFFSET) {
							continue;
						}
						vtables[col]->destroy(row_in_column(ci, col, row));
					}
				}
				chunk.count = 0;
				if (chunk.data != nullptr) {
					::operator delete(chunk.data, std::align_val_t { CHUNK_BLOCK_ALIGN });
					chunk.data = nullptr;
				}
			}
		}

		// Returns NO_COLUMN_INDEX if the archetype doesn't carry `id`.
		std::size_t column_index_for(component_type_id_t id) const {
			for (std::size_t i = 0; i < signature.size(); ++i) {
				if (signature[i] == id) {
					return i;
				}
			}
			return NO_COLUMN_INDEX;
		}

		bool has_component(component_type_id_t id) const {
			return column_index_for(id) != NO_COLUMN_INDEX;
		}

		// Both inputs sorted ascending; returns true iff `required ⊆ signature`.
		bool matches_all(std::vector<component_type_id_t> const& required) const {
			std::size_t i = 0;
			std::size_t j = 0;
			while (i < required.size() && j < signature.size()) {
				if (required[i] == signature[j]) {
					++i;
					++j;
				} else if (signature[j] < required[i]) {
					++j;
				} else {
					return false;
				}
			}
			return i == required.size();
		}

		// Both inputs sorted ascending; returns true iff `excluded ∩ signature == ∅`.
		bool matches_none(std::vector<component_type_id_t> const& excluded) const {
			std::size_t i = 0;
			std::size_t j = 0;
			while (i < excluded.size() && j < signature.size()) {
				if (excluded[i] == signature[j]) {
					return false;
				} else if (signature[j] < excluded[i]) {
					++j;
				} else {
					++i;
				}
			}
			return true;
		}

		// Returns the number of bytes of slab in a chunk that store EntityIDs (= chunk_capacity * sizeof(EntityID)).
		std::size_t entity_slab_bytes() const {
			return chunk_capacity * sizeof(EntityID);
		}

		EntityID* entity_array(std::size_t chunk_index) {
			return reinterpret_cast<EntityID*>(chunks[chunk_index].data);
		}
		EntityID const* entity_array(std::size_t chunk_index) const {
			return reinterpret_cast<EntityID const*>(chunks[chunk_index].data);
		}

		// Returns the base address of column `col`'s slab in the given chunk, or nullptr if
		// the column is a tag (no slab). Callers must not dereference for tag columns.
		void* column_array(std::size_t chunk_index, std::size_t col) {
			if (column_offsets[col] == NO_COLUMN_OFFSET) {
				return nullptr;
			}
			return chunks[chunk_index].data + column_offsets[col];
		}
		void const* column_array(std::size_t chunk_index, std::size_t col) const {
			if (column_offsets[col] == NO_COLUMN_OFFSET) {
				return nullptr;
			}
			return chunks[chunk_index].data + column_offsets[col];
		}

		// Returns a pointer to the column's slot at (chunk, row), or nullptr for tag columns.
		void* row_in_column(std::size_t chunk_index, std::size_t col, std::size_t row) {
			if (column_offsets[col] == NO_COLUMN_OFFSET) {
				return nullptr;
			}
			return chunks[chunk_index].data + column_offsets[col] + row * vtables[col]->size;
		}
		void const* row_in_column(std::size_t chunk_index, std::size_t col, std::size_t row) const {
			if (column_offsets[col] == NO_COLUMN_OFFSET) {
				return nullptr;
			}
			return chunks[chunk_index].data + column_offsets[col] + row * vtables[col]->size;
		}

		// Allocates a new fresh chunk with no rows. The chunk's `data` pointer is non-null.
		// Routes through chunk_pool when set; falls back to ::operator new otherwise (used
		// by tests that construct an Archetype bare, without a World).
		std::size_t allocate_chunk() {
			DataChunk fresh;
			if (chunk_pool != nullptr) {
				fresh.data = chunk_pool->acquire();
			} else {
				fresh.data = static_cast<unsigned char*>(
					::operator new(CHUNK_BLOCK_BYTES, std::align_val_t { CHUNK_BLOCK_ALIGN })
				);
			}
			chunks.push_back(std::move(fresh));
			return chunks.size() - 1;
		}

		// Drops the trailing chunk if it's empty, returning its block to the pool (or to
		// ::operator delete if no pool is wired). No-op if there are no chunks or the
		// trailing chunk still holds rows. No retain-one rule — a fully-drained archetype
		// has chunks.size() == 0, and the next reserve_row pulls a fresh block from the
		// pool at the same cost as the previous "retained spare" indexing.
		void drop_empty_trailing_chunk() {
			if (chunks.empty() || chunks.back().count != 0) {
				return;
			}
			DataChunk& back = chunks.back();
			if (chunk_pool != nullptr) {
				chunk_pool->release(back.data);
			} else if (back.data != nullptr) {
				::operator delete(back.data, std::align_val_t { CHUNK_BLOCK_ALIGN });
			}
			back.data = nullptr;
			chunks.pop_back();
		}

		// Destroys every live component and routes every chunk's block to the pool, then
		// clears the chunks vector and resets total_entity_count. Called explicitly by
		// World::~World before the archetypes vector destroys, so the (non-pool fallback)
		// Archetype destructor sees an empty chunks vector and has nothing to do.
		void drain_to_pool(ChunkPool& pool) {
			for (std::size_t ci = 0; ci < chunks.size(); ++ci) {
				DataChunk& chunk = chunks[ci];
				for (std::size_t row = 0; row < chunk.count; ++row) {
					for (std::size_t col = 0; col < signature.size(); ++col) {
						if (column_offsets[col] == NO_COLUMN_OFFSET) {
							continue;
						}
						vtables[col]->destroy(row_in_column(ci, col, row));
					}
				}
				chunk.count = 0;
				pool.release(chunk.data);
				chunk.data = nullptr;
			}
			chunks.clear();
			total_entity_count = 0;
		}

		// Reserve a new row at the end of storage — finds the first non-full chunk (or
		// allocates a new one), bumps that chunk's `count`, and returns (chunk_index, row).
		// Caller must placement-new component values into each non-tag column at this slot
		// AFTER calling reserve_row, then push the EntityID via `entity_array(chunk_index)[row] = eid`.
		// Bumps every column_version.
		struct RowLocation {
			std::size_t chunk_index;
			std::size_t row;
		};
		RowLocation reserve_row() {
			std::size_t chunk_index;
			if (chunks.empty() || chunks.back().count >= chunk_capacity) {
				chunk_index = allocate_chunk();
			} else {
				chunk_index = chunks.size() - 1;
			}
			std::size_t const row = chunks[chunk_index].count;
			++chunks[chunk_index].count;
			++total_entity_count;
			for (uint64_t& v : column_versions) {
				++v;
			}
			return { chunk_index, row };
		}

		// Returns the (chunk_index, row) of the global last row, used when swap-popping.
		// Precondition: total_entity_count > 0.
		RowLocation last_row_location() const {
			std::size_t const last_chunk = chunks.size() - 1;
			std::size_t const last_row = chunks[last_chunk].count - 1;
			return { last_chunk, last_row };
		}
	};
}
