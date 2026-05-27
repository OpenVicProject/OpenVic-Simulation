#include "openvic-simulation/ecs/Checksum.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "openvic-simulation/ecs/Archetype.hpp"
#include "openvic-simulation/ecs/ChecksumTraits.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

namespace OpenVic::ecs {

	// The one friend of World for checksum purposes (declared in World.hpp). Read-only:
	// walks `archetypes` and `singletons` directly, never the query cache.
	struct WorldChecksumAccess {
		// Single walk shared by world_checksum (out == nullptr) and
		// world_checksum_breakdown (entries filled).
		static uint64_t walk(World const& world, WorldChecksumBreakdown* out) {
			uint64_t total = CHECKSUM_SEED;

			// --- entity state: archetypes by index, chunks ascending, rows ascending ---
			for (std::size_t archetype_index = 0; archetype_index < world.archetypes.size(); ++archetype_index) {
				Archetype const& arch = world.archetypes[archetype_index];
				if (arch.total_entity_count == 0) {
					// Skip drained archetypes: live state is identical whether or not this
					// archetype ever existed, and a future loader would not recreate it.
					continue;
				}
				uint64_t h = CHECKSUM_SEED;
				for (component_type_id_t id : arch.signature) {
					h = fold_uint64(id, h);
				}
				for (std::size_t ci = 0; ci < arch.chunks.size(); ++ci) {
					std::size_t const count = arch.chunks[ci].count;
					EntityID const* eids = arch.entity_array(ci);
					for (std::size_t row = 0; row < count; ++row) {
						h = fold_uint64(eids[row].to_uint64(), h);
					}
					for (std::size_t col = 0; col < arch.signature.size(); ++col) {
						if (arch.vtables[col]->hash_rows == nullptr) {
							continue; // tag column — presence already folded via the signature
						}
						h = arch.vtables[col]->hash_rows(arch.column_array(ci, col), count, h);
					}
				}
				total = fold_uint64(h, total);
				if (out != nullptr) {
					out->archetype_entries.push_back(ArchetypeChecksumEntry {
						static_cast<uint32_t>(archetype_index), arch.signature, h
					});
				}
			}

			// --- singletons: map iteration order is nondeterministic, so fold in sorted
			// component-id order ---
			std::vector<std::pair<component_type_id_t, World::SingletonRecord const*>> sorted;
			sorted.reserve(world.singletons.size());
			for (std::pair<component_type_id_t const, World::SingletonRecord> const& entry : world.singletons) {
				sorted.emplace_back(entry.first, &entry.second);
			}
			std::sort(sorted.begin(), sorted.end(), [](
				std::pair<component_type_id_t, World::SingletonRecord const*> const& lhs,
				std::pair<component_type_id_t, World::SingletonRecord const*> const& rhs
			) {
				return lhs.first < rhs.first;
			});
			for (std::pair<component_type_id_t, World::SingletonRecord const*> const& entry : sorted) {
				uint64_t h = fold_uint64(entry.first, CHECKSUM_SEED);
				h = entry.second->checksum(entry.second->ptr.get(), h);
				total = fold_uint64(h, total);
				if (out != nullptr) {
					out->singleton_entries.push_back(SingletonChecksumEntry { entry.first, h });
				}
			}

			if (out != nullptr) {
				out->total = total;
			}
			return total;
		}
	};

	uint64_t world_checksum(World const& world) {
		return WorldChecksumAccess::walk(world, nullptr);
	}

	WorldChecksumBreakdown world_checksum_breakdown(World const& world) {
		WorldChecksumBreakdown breakdown;
		WorldChecksumAccess::walk(world, &breakdown);
		return breakdown;
	}

	uint64_t fold_checksum_breakdown(WorldChecksumBreakdown const& breakdown) {
		uint64_t total = CHECKSUM_SEED;
		for (ArchetypeChecksumEntry const& entry : breakdown.archetype_entries) {
			total = fold_uint64(entry.hash, total);
		}
		for (SingletonChecksumEntry const& entry : breakdown.singleton_entries) {
			total = fold_uint64(entry.hash, total);
		}
		return total;
	}
}
