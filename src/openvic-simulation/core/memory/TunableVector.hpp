#pragma once

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"
#include "openvic-simulation/core/stl/containers/TunableVector.hpp"

namespace OpenVic::memory {
	template<
		typename T, ::OpenVic::stl::_detail::tunable_growth_trait GrowthTrait = ::OpenVic::stl::default_growth_traits,
		class RawAllocator = foonathan::memory::default_allocator>
	using tunable_vector =
		::OpenVic::stl::tunable_vector<T, GrowthTrait, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;
}
