#pragma once

#include "openvic-simulation/Alias.hpp"
#include "openvic-simulation/core/object/FixedPoint.hpp"

namespace OpenVic {
	struct CountryInstance;
	struct Pop;

	struct Employee {
	private:
		Pop& PROPERTY_MOD(pop);
		pop_size_t PROPERTY_RW(size);
		fixed_point_t PROPERTY_RW(minimum_wage_cached);
	public:
		Employee(Pop& new_pop, const pop_size_t new_size);
		fixed_point_t update_minimum_wage(CountryInstance& country_to_report_economy);
	};
}