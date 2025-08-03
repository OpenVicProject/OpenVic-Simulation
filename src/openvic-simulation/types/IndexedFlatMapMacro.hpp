// depends on #include "openvic-simulation/types/IndexedFlatMap.hpp"
// depends on #include "openvic-simulation/utility/Getters.hpp"

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
