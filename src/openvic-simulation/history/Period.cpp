#include "Period.hpp"

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

bool Period::is_date_in_period(Date date) const {
	return start_date <= date && (!end_date.has_value() || end_date.value() >= date);
}

bool Period::try_set_end(Date date) {
	if (end_date.has_value()) {
		spdlog::error_s("Period already has end date {}", end_date.value());
		return false;
	}

	if (date < start_date) {
		spdlog::error_s("Proposed end date {} is before start date {}", date, start_date);
		return false;
	}

	end_date = date;
	return true;
}
