// depends on #include "openvic-simulation/types/UnitBranchType.hpp"

#define _UNIT_BRANCHED_GETTER(name, land, naval, const) \
	template<unit_branch_t Branch> \
	constexpr auto const& name() const { \
		if constexpr (Branch == unit_branch_t::LAND) { \
			return land; \
		} else if constexpr (Branch == unit_branch_t::NAVAL) { \
			return naval; \
		} \
	}

#define UNIT_BRANCHED_GETTER(name, land, naval) _UNIT_BRANCHED_GETTER(name, land, naval, )
#define UNIT_BRANCHED_GETTER_CONST(name, land, naval) _UNIT_BRANCHED_GETTER(name, land, naval, const)