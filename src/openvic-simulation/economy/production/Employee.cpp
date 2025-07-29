#include "Employee.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"

using namespace OpenVic;

Employee::Employee(Pop& new_pop, const pop_size_t new_size)
	: pop { new_pop },
	size { new_size }
	{}

fixed_point_t Employee::update_minimum_wage(CountryInstance& country_to_report_economy) {
	const fixed_point_t minimum_wage_base = country_to_report_economy.calculate_minimum_wage_base(*pop.get_type());
	return minimum_wage_cached = minimum_wage_base * size / Pop::size_denominator;
}