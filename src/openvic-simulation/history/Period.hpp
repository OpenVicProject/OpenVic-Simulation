#pragma once

#include <optional>
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/utility/Logger.hpp"

namespace OpenVic {
	struct Period {
	private:
		const Date start_date;
		std::optional<Date> end_date;

	public:
		Period(const Date new_start_date, const std::optional<Date> new_end_date);
		bool is_date_in_period(const Date date) const;
		bool try_set_end(const Date date);
	};
}