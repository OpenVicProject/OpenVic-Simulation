#pragma once

#include "openvic-simulation/core/container/IndexedFlatMap.hpp"
#include "openvic-simulation/utility/Getters.hpp" // IWYU pragma: keep
#include "openvic-simulation/utility/MemoryTracker.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::memory {
	template<
		typename ForwardedKeyType, typename ValueType, typename RawAllocator = foonathan::memory::default_allocator,
		typename Allocator = foonathan::memory::std_allocator<ValueType, memory::tracker<RawAllocator>>>
	using IndexedFlatMap = IndexedFlatMap<ForwardedKeyType, ValueType, Allocator>;
}

#define OV_IFLATMAP_PROPERTY(KEYTYPE, VALUETYPE, NAME) OV_IFLATMAP_PROPERTY_ACCESS(KEYTYPE, VALUETYPE, NAME, private)
#define OV_IFLATMAP_PROPERTY_ACCESS(KEYTYPE, VALUETYPE, NAME, ACCESS) \
	::OpenVic::memory::IndexedFlatMap<KEYTYPE, VALUETYPE> NAME; \
\
public: \
	[[nodiscard]] constexpr ::OpenVic::memory::IndexedFlatMap<KEYTYPE, VALUETYPE> const& get_##NAME() const { \
		return NAME; \
	} \
	[[nodiscard]] auto get_##NAME(KEYTYPE const& key) const \
		-> decltype(OpenVic::_get_property<VALUETYPE>(std::declval<std::add_const_t<VALUETYPE>>())); \
	ACCESS:
