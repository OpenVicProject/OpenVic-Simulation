#include "openvic-simulation/types/IndexedFlatMap.hpp" // IWYU pragma: keep for field type
#include "openvic-simulation/utility/Getters.hpp" // IWYU pragma: keep for _get_property

#define IndexedFlatMap_PROPERTY(KEYTYPE, VALUETYPE, NAME) IndexedFlatMap_PROPERTY_ACCESS(KEYTYPE, VALUETYPE, NAME, private)
#define IndexedFlatMap_PROPERTY_ACCESS(KEYTYPE, VALUETYPE, NAME, ACCESS) \
	IndexedFlatMap<KEYTYPE,VALUETYPE> NAME; \
\
public: \
	[[nodiscard]] constexpr IndexedFlatMap<KEYTYPE,VALUETYPE> const& get_##NAME() const { \
		return NAME; \
	} \
	[[nodiscard]] auto get_##NAME(KEYTYPE const& key) const -> decltype(OpenVic::_get_property<VALUETYPE>(NAME.at_index(0))); \
	ACCESS:
