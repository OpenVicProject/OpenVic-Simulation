#pragma once

#include <cstdint>

namespace OpenVic {
	enum struct colony_status_t : uint8_t { STATE, PROTECTORATE, COLONY };

	// This combines COLONY and PROTECTORATE statuses, as opposed to non-colonial STATE provinces
	static constexpr bool is_colonial(colony_status_t colony_status) {
		return colony_status != colony_status_t::STATE;
	}
}