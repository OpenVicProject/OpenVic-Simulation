#pragma once

#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct PopsAggregateDeps {
		ideology_index_t ideology_count;
		party_policy_index_t party_policy_count;
		pop_type_index_t pop_type_count;
		reform_index_t reform_count;
		strata_index_t strata_count;
	};
}