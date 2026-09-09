#pragma once

#include <cstdint>

namespace OpenVic::ecs {
	// High bit of EntityID::generation reserved for deferred-create placeholders. A placeholder
	// is `{ index = local_seq, generation = DEFERRED_GENERATION_BIT }` returned from
	// CommandBuffer::create_entity in parallel mode (inside a SystemThreaded body). The real
	// generation is assigned when the slot is allocated at apply time. Real generations stay in
	// [1, 0x7FFFFFFF] — World::allocate_entity_slot clamps over this range.
	inline constexpr uint32_t DEFERRED_GENERATION_BIT = 0x80000000u;

	struct EntityID {
		uint32_t index = 0;
		uint32_t generation = 0;

		constexpr bool operator==(EntityID const& rhs) const {
			return index == rhs.index && generation == rhs.generation;
		}

		constexpr bool operator!=(EntityID const& rhs) const {
			return !(*this == rhs);
		}

		// Generation 0 is the invalid sentinel — valid entities always have generation >= 1.
		// Deferred placeholders also satisfy is_valid(): their generation has DEFERRED_GENERATION_BIT
		// set (non-zero), but they are NOT yet alive in the World. Use is_deferred() to distinguish.
		constexpr bool is_valid() const {
			return generation != 0;
		}

		// True for a placeholder returned by CommandBuffer::create_entity in parallel mode that has
		// not yet been resolved to a real EntityID by CommandBuffer::apply. Public World accessors
		// treat deferred IDs as "not present" (return false / nullptr / 0). The placeholder is only
		// usable as an argument to other ops on the same CommandBuffer recording session.
		constexpr bool is_deferred() const {
			return (generation & DEFERRED_GENERATION_BIT) != 0;
		}

		constexpr uint64_t to_uint64() const {
			return (static_cast<uint64_t>(generation) << 32) | static_cast<uint64_t>(index);
		}

		static constexpr EntityID from_uint64(uint64_t value) {
			EntityID eid;
			eid.index = static_cast<uint32_t>(value & 0xFFFFFFFFULL);
			eid.generation = static_cast<uint32_t>(value >> 32);
			return eid;
		}
	};

	inline constexpr EntityID INVALID_ENTITY_ID = {};

	// Strong handle for an "immutable entity" — one created via World::create_immutable_entity
	// (or CommandBuffer::create_immutable_entity) that can never change archetype after creation.
	// Same layout as EntityID (index + generation) but a DISTINCT type with NO implicit conversion
	// to EntityID, so it cannot bind to the structural-mutation APIs (World/CommandBuffer
	// add_component / remove_component), all of which take EntityID by value. Passing an
	// ImmutableEntityID to those is a compile error — that is the compile-time immutability
	// guarantee. Read and data-mutation ops (get_component returns a mutable C*), is_alive, and
	// destroy_entity are reachable via explicit ImmutableEntityID overloads. The only bridge to a
	// mutable EntityID is unsafe_mutable_id(), named so every structural bypass is grep-able.
	struct ImmutableEntityID {
		uint32_t index = 0;
		uint32_t generation = 0;

		constexpr bool operator==(ImmutableEntityID const& rhs) const {
			return index == rhs.index && generation == rhs.generation;
		}

		constexpr bool operator!=(ImmutableEntityID const& rhs) const {
			return !(*this == rhs);
		}

		// Generation 0 is the invalid sentinel — valid handles always have generation >= 1.
		constexpr bool is_valid() const {
			return generation != 0;
		}

		// True for a deferred-create placeholder returned by CommandBuffer::create_immutable_entity
		// in parallel mode, before CommandBuffer::apply resolves it to a real handle. Mirrors
		// EntityID::is_deferred.
		constexpr bool is_deferred() const {
			return (generation & DEFERRED_GENERATION_BIT) != 0;
		}

		constexpr uint64_t to_uint64() const {
			return (static_cast<uint64_t>(generation) << 32) | static_cast<uint64_t>(index);
		}

		static constexpr ImmutableEntityID from_uint64(uint64_t value) {
			ImmutableEntityID id;
			id.index = static_cast<uint32_t>(value & 0xFFFFFFFFULL);
			id.generation = static_cast<uint32_t>(value >> 32);
			return id;
		}

		// The ONLY bridge to a mutable EntityID. Intentionally verbose so that every structural
		// bypass of the immutability guarantee is auditable by grepping "unsafe_mutable_id". There
		// is deliberately NO implicit conversion operator to EntityID.
		constexpr EntityID unsafe_mutable_id() const {
			return EntityID { index, generation };
		}
	};

	inline constexpr ImmutableEntityID INVALID_IMMUTABLE_ENTITY_ID = {};
}
