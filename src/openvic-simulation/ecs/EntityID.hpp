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
}
