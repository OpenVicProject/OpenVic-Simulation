#pragma once

#include "openvic-simulation/pop/Pop.hpp"

namespace OpenVic {
	struct Employee {
	private:
		Pop& pop;
		pop_size_t PROPERTY_RW(size);
		fixed_point_t PROPERTY_RW(minimum_wage_cached);
	public:
		constexpr Pop& get_pop() {
			return pop;
		}

		Employee(Pop& new_pop, const pop_size_t new_size);
		fixed_point_t update_minimum_wage(CountryInstance const& country_to_report_economy);
	};
}