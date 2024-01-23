#include "Date.hpp"

#include <cctype>

#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

using namespace OpenVic;

std::string Timespan::to_string() const {
	return std::to_string(days);
}

Timespan::operator std::string() const {
	return to_string();
}

std::ostream& OpenVic::operator<<(std::ostream& out, Timespan const& timespan) {
	return out << timespan.to_string();
}

std::string Date::to_string() const {
	std::stringstream ss;
	ss << *this;
	return ss.str();
}

Date::operator std::string() const {
	return to_string();
}

std::ostream& OpenVic::operator<<(std::ostream& out, Date date) {
	return out << static_cast<int>(date.get_year()) << Date::SEPARATOR_CHARACTER << static_cast<int>(date.get_month())
		<< Date::SEPARATOR_CHARACTER << static_cast<int>(date.get_day());
}

// Parsed from string of the form YYYY.MM.DD
Date Date::from_string(char const* const str, char const* const end, bool* successful, bool quiet) {
	if (successful != nullptr) {
		*successful = true;
	}

	year_t year = 0;
	month_t month = 1;
	day_t day = 1;

	if (str == nullptr || end <= str) {
		if (!quiet) {
			Logger::error(
				"Invalid string start/end pointers: ", static_cast<void const*>(str), " - ", static_cast<void const*>(end)
			);
		}
		if (successful != nullptr) {
			*successful = false;
		}
		return { year, month, day };
	}

	char const* year_end = str;
	while (std::isdigit(*year_end) && ++year_end < end) {}

	if (year_end <= str) {
		if (!quiet) {
			Logger::error("Failed to find year digits in date: ", std::string_view { str, static_cast<size_t>(end - str) });
		}
		if (successful != nullptr) {
			*successful = false;
		}
		return { year, month, day };
	}

	bool sub_successful = false;
	uint64_t val = StringUtils::string_to_uint64(str, year_end, &sub_successful, 10);
	if (!sub_successful || val > std::numeric_limits<year_t>::max()) {
		if (!quiet) {
			Logger::error("Failed to read year: ", std::string_view { str, static_cast<size_t>(end - str) });
		}
		if (successful != nullptr) {
			*successful = false;
		}
		return { year, month, day };
	}
	year = val;
	if (year_end < end) {
		if (*year_end == SEPARATOR_CHARACTER) {
			char const* const month_start = year_end + 1;
			char const* month_end = month_start;
			if (month_start < end) {
				while (std::isdigit(*month_end) && ++month_end < end) {}
			}
			if (month_start >= month_end) {
				if (!quiet) {
					Logger::error(
						"Failed to find month digits in date: ", std::string_view { str, static_cast<size_t>(end - str) }
					);
				}
				if (successful != nullptr) {
					*successful = false;
				}
			} else {
				sub_successful = false;
				val = StringUtils::string_to_uint64(month_start, month_end, &sub_successful, 10);
				if (!sub_successful || val < 1 || val > MONTHS_IN_YEAR) {
					if (!quiet) {
						Logger::error("Failed to read month: ", std::string_view { str, static_cast<size_t>(end - str) });
					}
					if (successful != nullptr) {
						*successful = false;
					}
				} else {
					month = val;
					if (month_end < end) {
						if (*month_end == SEPARATOR_CHARACTER) {
							char const* const day_start = month_end + 1;
							char const* day_end = day_start;
							if (day_start < end) {
								while (std::isdigit(*day_end) && ++day_end < end) {}
							}
							if (day_start >= day_end) {
								if (!quiet) {
									Logger::error(
										"Failed to find day digits in date: ",
										std::string_view { str, static_cast<size_t>(end - str) }
									);
								}
								if (successful != nullptr) {
									*successful = false;
								}
							} else {
								sub_successful = false;
								val = StringUtils::string_to_uint64(day_start, day_end, &sub_successful);
								if (!sub_successful || val < 1 || val > DAYS_IN_MONTH[month - 1]) {
									if (!quiet) {
										Logger::error(
											"Failed to read day: ", std::string_view { str, static_cast<size_t>(end - str) }
										);
									}
									if (successful != nullptr) {
										*successful = false;
									}
								} else {
									day = val;
									if (day_end < end) {
										if (!quiet) {
											Logger::error(
												"Unexpected string \"",
												std::string_view { day_end, static_cast<size_t>(end - day_end) },
												"\" at the end of date ",
												std::string_view { str, static_cast<size_t>(end - str) }
											);
										}
										if (successful != nullptr) {
											*successful = false;
										}
									}
								}
							}
						} else {
							if (!quiet) {
								Logger::error(
									"Unexpected character \"", *month_end, "\" in month of date ",
									std::string_view { str, static_cast<size_t>(end - str) }
								);
							}
							if (successful != nullptr) {
								*successful = false;
							}
						}
					}
				}
			}
		} else {
			if (!quiet) {
				Logger::error(
					"Unexpected character \"", *year_end, "\" in year of date ",
					std::string_view { str, static_cast<size_t>(end - str) }
				);
			}
			if (successful != nullptr) {
				*successful = false;
			}
		}
	}
	return { year, month, day };
};

Date Date::from_string(char const* str, size_t length, bool* successful, bool quiet) {
	return from_string(str, str + length, successful, quiet);
}

Date Date::from_string(std::string_view str, bool* successful, bool quiet) {
	return from_string(str.data(), str.length(), successful, quiet);
}
