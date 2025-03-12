#pragma once

#include "openvic-simulation/economy/production/Employee.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct ProvinceInstance;
	struct ModifierEffectCache;
	struct SellResult;

	struct ResourceGatheringOperation {
	private:
		MarketInstance& market_instance;
		ProvinceInstance* location_ptr = nullptr;
		pop_size_t total_owner_count_in_state_cache = 0;
		pop_size_t total_worker_count_in_province_cache = 0;
		std::vector<Pop*> const* owner_pops_cache_nullable = nullptr;

		ProductionType const* PROPERTY_RW(production_type_nullable);
		fixed_point_t PROPERTY(revenue_yesterday);
		fixed_point_t PROPERTY(output_quantity_yesterday);
		fixed_point_t PROPERTY(unsold_quantity_yesterday);
		fixed_point_t PROPERTY_RW(size_multiplier);
		std::vector<Employee> PROPERTY(employees);
		pop_size_t PROPERTY(max_employee_count_cache, 0);
		pop_size_t PROPERTY(total_employees_count_cache, 0);
		pop_size_t PROPERTY(total_paid_employees_count_cache, 0);
		fixed_point_t PROPERTY(total_owner_income_cache);
		fixed_point_t PROPERTY(total_employee_income_cache);
		IndexedMap<PopType, pop_size_t> PROPERTY(employee_count_per_type_cache);

		fixed_point_t calculate_size_modifier() const;
		void hire();
		fixed_point_t produce();
		void pay_employees();
		static void after_sell(void* actor, SellResult const& sell_result);

	public:
		ResourceGatheringOperation(
			MarketInstance& new_market_instance,
			ProductionType const* new_production_type_nullable,
			fixed_point_t new_size_multiplier,
			fixed_point_t new_revenue_yesterday,
			fixed_point_t new_output_quantity_yesterday,
			fixed_point_t new_unsold_quantity_yesterday,
			std::vector<Employee>&& new_employees,
			decltype(employee_count_per_type_cache)::keys_type const& pop_type_keys
		);

		ResourceGatheringOperation(
			MarketInstance& new_market_instance,
			decltype(employee_count_per_type_cache)::keys_type const& pop_type_keys
		);

		constexpr bool is_valid() const {
			return production_type_nullable != nullptr;
		}
		void setup_location_ptr(ProvinceInstance& location);
		void initialise_rgo_size_multiplier();
		void rgo_tick();
	};
}
