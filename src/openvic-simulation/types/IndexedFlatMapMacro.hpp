#define IndexedFlatMap_PROPERTY(KEYTYPE, VALUETYPE, NAME) IndexedFlatMap_PROPERTY_ACCESS(KEYTYPE, VALUETYPE, NAME, private)
#define IndexedFlatMap_PROPERTY_ACCESS(KEYTYPE, VALUETYPE, NAME, ACCESS) \
	IndexedFlatMap<KEYTYPE,VALUETYPE> NAME; \
\
public: \
	[[nodiscard]] constexpr IndexedFlatMap<KEYTYPE,VALUETYPE> const& get_##NAME() const { \
		return NAME; \
	} \
	ACCESS:
