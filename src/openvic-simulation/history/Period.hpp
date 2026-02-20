#pragma once

#include <optional>

#include "openvic-simulation/types/Date.hpp"

namespace OpenVic {
	struct Period {
	private:
		const Date start_date;
		std::optional<Date> end_date;

	public:
		constexpr Period(const Date new_start_date, const std::optional<const Date> new_end_date)
			: start_date { new_start_date }, end_date { new_end_date } {}

		bool is_date_in_period(Date date) const;
		bool try_set_end(Date date);
	};
}
