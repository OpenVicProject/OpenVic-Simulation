#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

// Hashing primitives + the per-type checksum contract behind ecs/Checksum.hpp's full-state
// world checksum. Lives in its own dependency-free header because both Archetype.hpp
// (column vtable hash thunks) and World.hpp (singleton hash thunks) need it, while
// Checksum.hpp includes World.hpp.
//
// === The universal hashing rule ===
// Every component or singleton type C hashes exactly one of two ways:
//   1. Raw bytes — allowed only if std::has_unique_object_representations_v<C>, i.e. the
//      compiler inserted NO padding. Padding bytes are indeterminate garbage; zero-initialized
//      chunk memory does NOT save you, because whole-struct copies from stack temporaries
//      import the source's garbage padding. Fill gaps with explicit `_pad` members, zeroed
//      at construction.
//   2. A custom hash: a free function `uint64_t ecs_checksum(C const&, uint64_t seed)`
//      declared at C's scope (found by ADL), BEFORE C's first World/CommandBuffer use in the
//      translation unit. MANDATORY for anything holding heap data (memory::vector members,
//      std::string, ring buffers, ...). The custom hash must walk the heap contents
//      deterministically: sizes + elements in index order — never capacities or addresses.
//      A custom hash takes precedence over the byte path, so a uniquely-representable type
//      may still provide one (e.g. to ignore scratch fields).
// A type satisfying neither is a compile error at registration/use — never a silent skip.
//
// === The one rule the compiler cannot check ===
// Raw pointers ARE uniquely representable, so a byte-hashed type must NEVER contain one:
// hashing addresses is nondeterministic garbage across runs and silently breaks every
// determinism gate (worker-count invariance, golden runs, save/load). Components should not
// hold raw pointers anyway — references are dense indices or EntityID (architecture
// convention C1). If a type must hold a pointer, give it a custom ecs_checksum that hashes
// the pointee's deterministic content (or mere presence), never the address.

namespace OpenVic::ecs {

	// Same constants as fnv1a_64 in ComponentTypeID.hpp — one FNV-1a definition project-wide.
	inline constexpr uint64_t CHECKSUM_FNV_PRIME = 0x100000001b3ULL;
	// Every checksum fold starts from the FNV-1a 64-bit offset basis.
	inline constexpr uint64_t CHECKSUM_SEED = 0xcbf29ce484222325ULL;

	// FNV-1a over a raw byte range, continuing from `seed`. Byte-sequential, so hashing one
	// contiguous slab in a single call is bit-identical to hashing its elements one by one.
	inline uint64_t fnv1a_64_bytes(void const* data, std::size_t size, uint64_t seed) {
		unsigned char const* bytes = static_cast<unsigned char const*>(data);
		uint64_t h = seed;
		for (std::size_t i = 0; i < size; ++i) {
			h ^= bytes[i];
			h *= CHECKSUM_FNV_PRIME;
		}
		return h;
	}

	// Endian-independent fold of one 64-bit value, low byte first.
	constexpr uint64_t fold_uint64(uint64_t value, uint64_t seed) {
		uint64_t h = seed;
		for (int i = 0; i < 8; ++i) {
			h ^= (value >> (i * 8)) & 0xFFULL;
			h *= CHECKSUM_FNV_PRIME;
		}
		return h;
	}

	// Detects the custom-hash convention: ADL free function
	//   uint64_t ecs_checksum(C const&, uint64_t seed)
	// void_t detection trait, not requires{} — MSVC mishandles ill-formed expressions inside
	// requires-expressions (see tests/src/ecs/ImmutableEntity.cpp).
	template<typename C, typename = void>
	struct has_custom_checksum : std::false_type {};
	template<typename C>
	struct has_custom_checksum<C,
		std::void_t<decltype(ecs_checksum(std::declval<C const&>(), std::declval<uint64_t>()))>>
		: std::true_type {};

	template<typename C>
	inline constexpr bool has_custom_checksum_v = has_custom_checksum<C>::value;

	// Tags (std::is_empty) are exempt: their contribution is presence only, carried by the
	// archetype signature / singleton id fold. Custom hash takes precedence over byte path.
	template<typename C>
	inline constexpr bool is_checksummable_v =
		std::is_empty_v<C> || has_custom_checksum_v<C> || std::has_unique_object_representations_v<C>;

#define OPENVIC_ECS_CHECKSUM_ENFORCE_MESSAGE \
	"ECS checksum: this component/singleton type cannot be hashed. Fix one of: " \
	"(a) make it uniquely representable - no float/double members, no implicit compiler " \
	"padding (fill gaps with explicit '_pad' members, zeroed at construction); or " \
	"(b) provide 'uint64_t ecs_checksum(C const&, uint64_t seed)' as a free function at C's " \
	"scope, declared before C's first World/CommandBuffer use in the translation unit - " \
	"MANDATORY for heap-holding types (memory::vector / std::string / unique_ptr members): " \
	"walk sizes + elements in index order, never capacities or addresses. WARNING: a " \
	"byte-hashed type must NEVER contain a raw pointer - pointer bytes are uniquely " \
	"representable but nondeterministic across runs and silently break every determinism gate."

	using checksum_rows_fn = uint64_t (*)(void const* column, std::size_t count, uint64_t seed);
	using checksum_value_fn = uint64_t (*)(void const* value, uint64_t seed);

	// Hash one value of C into `seed`: custom path > byte path; tags fold nothing.
	template<typename C>
	uint64_t checksum_value(C const& value, uint64_t seed) {
		static_assert(is_checksummable_v<C>, OPENVIC_ECS_CHECKSUM_ENFORCE_MESSAGE);
		if constexpr (std::is_empty_v<C>) {
			(void) value;
			return seed;
		} else if constexpr (has_custom_checksum_v<C>) {
			static_assert(
				std::is_same_v<
					decltype(ecs_checksum(std::declval<C const&>(), std::declval<uint64_t>())), uint64_t>,
				"ecs_checksum(C const&, uint64_t seed) must return uint64_t"
			);
			return ecs_checksum(value, seed);
		} else {
			return fnv1a_64_bytes(&value, sizeof(C), seed);
		}
	}

	// Per-column bulk hash thunk, stored in ColumnVTable::hash_rows. Never built for tags
	// (tag columns carry nullptr). This is the enforcement point for entity components:
	// instantiating column_vtable_for<C>() static_asserts the universal rule for C.
	template<typename C>
	checksum_rows_fn checksum_rows_thunk_for() {
		static_assert(!std::is_empty_v<C>, "tag columns carry no hash thunk");
		static_assert(is_checksummable_v<C>, OPENVIC_ECS_CHECKSUM_ENFORCE_MESSAGE);
		if constexpr (has_custom_checksum_v<C>) {
			return [](void const* column, std::size_t count, uint64_t seed) -> uint64_t {
				C const* elems = static_cast<C const*>(column);
				uint64_t h = seed;
				for (std::size_t i = 0; i < count; ++i) {
					h = checksum_value<C>(elems[i], h);
				}
				return h;
			};
		} else {
			// Whole-slab single call: bit-identical to per-element folding, because FNV-1a is
			// byte-sequential and uniquely-representable elements sit contiguously at a
			// sizeof(C) stride with no inter-element gap.
			return [](void const* column, std::size_t count, uint64_t seed) -> uint64_t {
				return fnv1a_64_bytes(column, count * sizeof(C), seed);
			};
		}
	}

	// Per-singleton hash thunk, captured by World::set_singleton<C> — the enforcement point
	// for singleton types. Tag singletons fold nothing (presence is the id fold in the walk).
	template<typename C>
	checksum_value_fn checksum_singleton_thunk_for() {
		if constexpr (std::is_empty_v<C>) {
			return [](void const*, uint64_t seed) -> uint64_t {
				return seed;
			};
		} else {
			static_assert(is_checksummable_v<C>, OPENVIC_ECS_CHECKSUM_ENFORCE_MESSAGE);
			return [](void const* value, uint64_t seed) -> uint64_t {
				return checksum_value<C>(*static_cast<C const*>(value), seed);
			};
		}
	}
}

// Author-asserted byte hashing for types std::has_unique_object_representations_v rejects
// conservatively (float/double members). Expand at Type's scope, immediately after its
// definition (works inside anonymous namespaces — ADL finds the overload there; deliberately
// NOT namespace-wrapped for that reason). By expanding this the author asserts: NO raw-pointer
// members, NO implicit padding bytes (the macro cannot detect padding — it is indeterminate
// garbage and WILL produce unstable checksums), and float values produced deterministically.
#define ECS_CHECKSUM_BYTES(Type) \
	inline uint64_t ecs_checksum(Type const& ecs_checksum_value_, uint64_t ecs_checksum_seed_) { \
		static_assert( \
			std::is_trivially_copyable_v<Type>, "ECS_CHECKSUM_BYTES requires a trivially copyable type" \
		); \
		return ::OpenVic::ecs::fnv1a_64_bytes(&ecs_checksum_value_, sizeof(Type), ecs_checksum_seed_); \
	}
