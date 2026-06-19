#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"

namespace OpenVic::ecs {
	enum class AccessMode : uint8_t {
		Read = 0,
		Write = 1
	};

	struct ComponentAccess {
		component_type_id_t component_id = 0;
		AccessMode mode = AccessMode::Read;

		constexpr bool operator==(ComponentAccess const& rhs) const {
			return component_id == rhs.component_id && mode == rhs.mode;
		}
	};

	// Returns true if `a` and `b` overlap in a way that requires ordering — i.e. at least
	// one of them writes a component the other reads or writes. Read/Read is fine.
	// Inputs are not required to be sorted.
	inline bool access_overlaps(std::span<ComponentAccess const> a, std::span<ComponentAccess const> b) {
		for (ComponentAccess const& x : a) {
			for (ComponentAccess const& y : b) {
				if (x.component_id != y.component_id) {
					continue;
				}
				if (x.mode == AccessMode::Write || y.mode == AccessMode::Write) {
					return true;
				}
			}
		}
		return false;
	}

	// Returns the list of component ids that overlap (with at least one Write). For
	// diagnostic messages: "system X and Y conflict on component {names}".
	inline std::vector<component_type_id_t> access_conflict_components(
		std::span<ComponentAccess const> a, std::span<ComponentAccess const> b
	) {
		std::vector<component_type_id_t> out;
		for (ComponentAccess const& x : a) {
			for (ComponentAccess const& y : b) {
				if (x.component_id != y.component_id) {
					continue;
				}
				if (x.mode == AccessMode::Write || y.mode == AccessMode::Write) {
					out.push_back(x.component_id);
					break;
				}
			}
		}
		std::sort(out.begin(), out.end());
		out.erase(std::unique(out.begin(), out.end()), out.end());
		return out;
	}

	// Folds an `extra_reads` array (a list of bare component_type_id_t with implicit Read
	// mode) into a flat AccessSet vector. Used by the scheduler when assembling each
	// system's full access set.
	inline void merge_extra_reads(
		std::vector<ComponentAccess>& dest, std::span<component_type_id_t const> extra
	) {
		for (component_type_id_t id : extra) {
			// If a Write entry already exists for this id, keep it (W+R coalesces to W).
			bool found_write = false;
			for (ComponentAccess const& e : dest) {
				if (e.component_id == id && e.mode == AccessMode::Write) {
					found_write = true;
					break;
				}
			}
			if (!found_write) {
				dest.push_back(ComponentAccess { id, AccessMode::Read });
			}
		}
	}

	// Coalesces duplicates: same id with W+R becomes W. Sorts by component_id then by mode
	// (Write first, so the Write entry wins on dedupe).
	inline void canonicalise_access_set(std::vector<ComponentAccess>& set) {
		std::sort(set.begin(), set.end(), [](ComponentAccess const& a, ComponentAccess const& b) {
			if (a.component_id != b.component_id) {
				return a.component_id < b.component_id;
			}
			// Write before Read so dedupe keeps the Write.
			return static_cast<uint8_t>(a.mode) > static_cast<uint8_t>(b.mode);
		});
		set.erase(
			std::unique(
				set.begin(), set.end(),
				[](ComponentAccess const& a, ComponentAccess const& b) {
					return a.component_id == b.component_id;
				}
			),
			set.end()
		);
	}
}
