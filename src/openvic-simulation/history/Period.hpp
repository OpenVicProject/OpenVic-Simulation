#pragma once

#include <optional>

#include "openvic-simulation/core/object/Date.hpp"

namespace OpenVic {
	struct Period {
	private:
		const Date start_date;
		std::optional<Date> end_date;

	public:
		Period(Date new_start_date, std::optional<Date> new_end_date);

		bool is_date_in_period(Date date) const;
		bool try_set_end(Date date);
	};
}
