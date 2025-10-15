#pragma once

#include <cstdint>
#include <limits>

namespace OpenVic {
	using client_id_type = uint64_t;
	static constexpr client_id_type MP_HOST_ID = static_cast<client_id_type>(~0);
	static constexpr client_id_type MP_INVALID_CLIENT_ID = std::numeric_limits<client_id_type>::max() - 1;
}
