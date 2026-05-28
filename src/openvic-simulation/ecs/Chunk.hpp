#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace OpenVic::ecs {

	// Fixed 16 KB chunk size, matching decs's `BLOCK_MEMORY_16K`. Chunks are the unit of
	// growth — when an archetype runs out of capacity in its current chunks, a fresh chunk
	// is allocated rather than relocating the existing column data. That's the principal
	// performance advantage over per-column std::vector storage.
	constexpr std::size_t CHUNK_BLOCK_BYTES = 16 * 1024;

	// Alignment for the chunk's heap block. Generous enough to cover EntityID and any
	// reasonable component (cache-line aligned for iteration efficiency).
	constexpr std::size_t CHUNK_BLOCK_ALIGN = 64;

	// `restrict` for typed pointers that loop bodies read/write. Tells the compiler the
	// pointee is not reached by any other pointer in the enclosing scope — without this
	// promise, writes through one column's pointer are assumed to potentially alias reads
	// through another column's pointer (because both pointers trace back to the same
	// `unsigned char*` chunk block), which blocks register-promotion and reordering in
	// the hot inner loops of the system drivers. Honored most reliably when applied to a
	// local declaration; weaker on function returns. Not standard C++ — each target
	// compiler spells it differently.
#if defined(_MSC_VER)
#	define OV_RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
#	define OV_RESTRICT __restrict__
#else
#	define OV_RESTRICT
#endif

	// Passive holder for one chunk's 16 KB block. Lifecycle is managed explicitly at every
	// callsite that owns a chunk:
	//   - Archetype::allocate_chunk calls ChunkPool::acquire and stores the result in `data`.
	//   - Archetype::drop_empty_trailing_chunk / World::compact_archetype_after_external_move
	//     call ChunkPool::release(data) and null `data` before pop_back.
	//   - Archetype::drain_to_pool (called from ~World) walks every chunk and releases.
	// There is intentionally no destructor here — pool routing must be visible at the
	// callsite, not hidden in RAII. Leaving `data` non-null when a DataChunk is destroyed
	// is a programmer error; the move-assign assert catches the common case where a moved-
	// into slot already held a live block, and the Archetype destructor catches the rest
	// (it ::operator delete's any leftover data as a non-pool fallback for bare-Archetype
	// test paths — but the pool-driven path drains chunks before Archetype destruction).
	//
	// Layout of the block (computed once at archetype creation, identical across every
	// chunk owned by that archetype):
	//
	//   [entity_id slab: EntityID[chunk_capacity]]
	//   [component_0 slab: aligned to vtable[0]->align, chunk_capacity * vtable[0]->size]
	//   [component_1 slab: ...]
	//   ...
	//
	// `count` is rows-currently-in-this-chunk; `chunk_capacity` is rows-per-chunk for the
	// owning archetype (constant for the chunk's lifetime). Tag (zero-size) columns get a
	// sentinel offset (size_t(-1)) and contribute no slab — they are tracked at the
	// archetype level only via `column_versions`.
	struct DataChunk {
		unsigned char* data = nullptr;
		std::size_t count = 0;

		DataChunk() = default;
		DataChunk(DataChunk const&) = delete;
		DataChunk& operator=(DataChunk const&) = delete;

		DataChunk(DataChunk&& other) noexcept : data { other.data }, count { other.count } {
			other.data = nullptr;
			other.count = 0;
		}
		DataChunk& operator=(DataChunk&& other) noexcept {
			if (this != &other) {
				// The destination must have been drained first — overwriting a live block
				// here would silently leak its 16 KB.
				assert(data == nullptr && "DataChunk move-assign over live block");
				data = other.data;
				count = other.count;
				other.data = nullptr;
				other.count = 0;
			}
			return *this;
		}
	};
}
