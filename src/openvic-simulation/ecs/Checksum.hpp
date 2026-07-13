#pragma once

#include <cstdint>
#include <vector>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"

namespace OpenVic::ecs {

	// === Full-state checksum ===
	// Deterministic 64-bit FNV-1a digest of all live ECS state: every live entity row
	// (EntityIDs + component bytes/custom hashes + tag presence via archetype signatures)
	// plus every singleton. This is the measuring instrument behind the project's
	// determinism gates: worker-count invariance, golden-run regression, and — once a save
	// system exists — the save/load gate.
	//
	// Canonical walk order is plain memory order (canonical in this project: packing is
	// deterministic within a run and across worker counts, and the future save system will
	// serialize entities in memory order and deserialize in order, reproducing packing
	// exactly):
	//   - archetypes by index (archetypes with no live entities are SKIPPED, so the digest
	//     is insensitive to dead archetype-creation history a loader would not replay);
	//   - per archetype: sorted signature first (tags contribute here, presence only), then
	//     chunks ascending — per chunk the EntityID (index, generation) pairs row-ascending,
	//     then component data column by column via ColumnVTable::hash_rows;
	//   - after all entity state: singletons in ascending component-id order, regardless of
	//     set_singleton call order.
	//
	// Per-type hashing follows the universal rule in ChecksumTraits.hpp (byte path requires
	// std::has_unique_object_representations_v, heap-holding types require a custom
	// ecs_checksum, anything else is a compile error at registration/use). EntityID-typed
	// reference fields inside components hash raw — ids are save-stable.
	//
	// Read-only by construction: walks the archetype vector directly — never touches the
	// query cache, never mutates the World. Equal totals on same-endian platforms imply
	// equal live state under the canonical walk (the byte path hashes slabs in native
	// endianness; cross-endian comparison is out of scope, matching the project).

	struct ArchetypeChecksumEntry {
		// Index into the World's archetype vector (memory order = canonical walk order).
		uint32_t archetype_index = 0;
		// Sorted component ids — copied so breakdown dumps are self-describing.
		std::vector<component_type_id_t> signature;
		// Self-contained: folded from CHECKSUM_SEED over the signature, then the rows.
		uint64_t hash = 0;
	};

	struct SingletonChecksumEntry {
		component_type_id_t id = 0;
		// Self-contained: folded from CHECKSUM_SEED over the id, then the value.
		uint64_t hash = 0;
	};

	// Per-archetype / per-singleton sub-hashes for divergence debugging — when two checksums
	// differ, the first question is always "where".
	struct WorldChecksumBreakdown {
		std::vector<ArchetypeChecksumEntry> archetype_entries; // memory order, empties skipped
		std::vector<SingletonChecksumEntry> singleton_entries; // ascending component id
		uint64_t total = 0;
	};

	// The checksum.
	uint64_t world_checksum(World const& world);

	// Same walk, with the per-part breakdown filled in.
	// Invariant: result.total == world_checksum(world) == fold_checksum_breakdown(result).
	WorldChecksumBreakdown world_checksum_breakdown(World const& world);

	// Recomputes the total from the entries alone (fold entry hashes in stored order,
	// archetypes then singletons, starting from CHECKSUM_SEED).
	uint64_t fold_checksum_breakdown(WorldChecksumBreakdown const& breakdown);
}
