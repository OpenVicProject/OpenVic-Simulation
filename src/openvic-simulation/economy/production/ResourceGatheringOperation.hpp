#pragma once

#include "openvic-simulation/economy/production/Employee.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/pop/Pop.hpp"

namespace OpenVic {
	struct ResourceGatheringOperation {
	private:
		ProductionType const* PROPERTY(production_type_nullable);
		fixed_point_t PROPERTY(revenue_yesterday);
		fixed_point_t PROPERTY(output_quantity_yesterday);
		fixed_point_t PROPERTY(unsold_quantity_yesterday);
		fixed_point_t PROPERTY_RW(size_multiplier);
		std::vector<Employee> PROPERTY(employees);
		Pop::pop_size_t PROPERTY(max_employee_count_cache);
		Pop::pop_size_t PROPERTY(total_employees_count_cache);
		Pop::pop_size_t PROPERTY(total_paid_employees_count_cache);
		fixed_point_t PROPERTY(total_owner_income_cache);
		fixed_point_t PROPERTY(total_employee_income_cache);
		IndexedMap<PopType, Pop::pop_size_t> PROPERTY(employee_count_per_type_cache);

		Pop::pop_size_t update_size_and_return_total_worker_count(ProvinceInstance& location);
		void hire(ProvinceInstance& location, const Pop::pop_size_t available_worker_count);
		fixed_point_t produce(
			ProvinceInstance& location,
			std::vector<Pop*>& owner_pops_cache,
			Pop::pop_size_t& total_owner_count_in_state_cache
		);
		void pay_employees(
			ProvinceInstance& location,
			const fixed_point_t revenue,
			const Pop::pop_size_t total_worker_count_in_province,
			std::vector<Pop*>& owner_pops_cache,
			Pop::pop_size_t const& total_owner_count_in_state_cache
		);

	public:
		ResourceGatheringOperation(
			ProductionType const* new_production_type_nullable,
			fixed_point_t new_size_multiplier,
			fixed_point_t new_revenue_yesterday,
			fixed_point_t new_output_quantity_yesterday,
			fixed_point_t new_unsold_quantity_yesterday,
			std::vector<Employee>&& new_employees,
			decltype(employee_count_per_type_cache)::keys_t const& pop_type_keys
		);
		ResourceGatheringOperation(decltype(employee_count_per_type_cache)::keys_t const& pop_type_keys);
		constexpr bool is_valid() const {
			return production_type_nullable != nullptr;
		}
		void set_production_type_nullable(ProductionType const* new_production_type_nullable);
		void initialise_for_new_game(ProvinceInstance& location);
	};
}
