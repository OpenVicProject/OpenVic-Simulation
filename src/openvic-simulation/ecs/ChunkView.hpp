#pragma once

#include <array>
#include <cstddef>
#include <type_traits>

#include "openvic-simulation/ecs/EntityID.hpp"

namespace OpenVic::ecs {

	// Lightweight view passed to `for_each_chunk` lambdas. Wraps a single chunk's worth of
	// component data: an EntityID array and one raw component-array pointer per Cs... in the
	// caller's argument list. All arrays share the same length, `count()`. Tag (zero-size)
	// component arrays are nullptr — callers must not dereference them.
	//
	// The view is valid only inside the `for_each_chunk` callback — the underlying chunk
	// data may be relocated by any subsequent structural mutation of the World.
	template<typename... Cs>
	struct ChunkView {
		std::size_t row_count = 0;
		EntityID* eids = nullptr;
		// One raw pointer per Cs... in declared order. Tag types map to nullptr.
		std::array<void*, sizeof...(Cs)> raw_arrays {};

		std::size_t count() const {
			return row_count;
		}

		EntityID* entities() {
			return eids;
		}
		EntityID const* entities() const {
			return eids;
		}

		// Returns the component slab for type C — must match exactly one of Cs...
		// For tag types this returns nullptr (no per-row data is stored).
		template<typename C>
		C* array() {
			constexpr std::size_t idx = index_of<C>();
			static_assert(idx < sizeof...(Cs), "ChunkView::array<C>: C is not in this view's component list");
			return static_cast<C*>(raw_arrays[idx]);
		}

		template<typename C>
		C const* array() const {
			constexpr std::size_t idx = index_of<C>();
			static_assert(idx < sizeof...(Cs), "ChunkView::array<C>: C is not in this view's component list");
			return static_cast<C const*>(raw_arrays[idx]);
		}

	private:
		template<typename C, std::size_t I = 0, typename First, typename... Rest>
		static constexpr std::size_t index_of_impl() {
			if constexpr (std::is_same_v<C, First>) {
				return I;
			} else if constexpr (sizeof...(Rest) == 0) {
				return sizeof...(Cs); // not found
			} else {
				return index_of_impl<C, I + 1, Rest...>();
			}
		}

		template<typename C>
		static constexpr std::size_t index_of() {
			return index_of_impl<C, 0, Cs...>();
		}
	};
}
