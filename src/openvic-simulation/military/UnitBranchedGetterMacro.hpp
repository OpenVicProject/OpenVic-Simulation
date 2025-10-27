#include "openvic-simulation/types/UnitBranchType.hpp" // IWYU pragma: keep for unit_branch_t

#define _OV_UNIT_BRANCHED_GETTER(name, land, naval, const) \
	template<unit_branch_t Branch> \
	constexpr auto const& name() const { \
		if constexpr (Branch == unit_branch_t::LAND) { \
			return land; \
		} else if constexpr (Branch == unit_branch_t::NAVAL) { \
			return naval; \
		} \
	}

#define OV_UNIT_BRANCHED_GETTER(name, land, naval) _OV_UNIT_BRANCHED_GETTER(name, land, naval, )
#define OV_UNIT_BRANCHED_GETTER_CONST(name, land, naval) _OV_UNIT_BRANCHED_GETTER(name, land, naval, const)
