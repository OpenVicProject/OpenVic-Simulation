#include "openvic-simulation/history/Period.hpp"

using namespace OpenVic;

Period::Period(
	const Date new_start_date,
	const std::optional<Date> new_end_date
) : start_date {new_start_date}, end_date {new_end_date} {}

bool Period::is_date_in_period(const Date date) const {
	return start_date <= date && (!end_date.has_value() || end_date.value() >= date);
}

bool Period::try_set_end(const Date date) {
	if (end_date.has_value()) {
		Logger::error("Period already has end date ", end_date.value());
		return false;
	}

	if (date < start_date) {
		Logger::error("Proposed end date ", date, " is before start date ", start_date);
		return false;
	}

	end_date = date;
	return true;
}