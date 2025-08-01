#pragma once

#include "openvic-simulation/economy/production/Employee.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/IndexedFlatMapMacro.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct MarketInstance;
	struct ModifierEffectCache;
	struct Pop;
	struct PopType;
	struct ProductionType;
	struct ProvinceInstance;
	struct SellResult;

	struct ResourceGatheringOperation {
	private:
		MarketInstance& market_instance;
		ProvinceInstance* location_ptr = nullptr;
		pop_size_t total_owner_count_in_state_cache = 0;
		pop_size_t total_worker_count_in_province_cache = 0;
		memory::vector<Pop*> const* owner_pops_cache_nullable = nullptr;

		ProductionType const* PROPERTY_RW(production_type_nullable);
		fixed_point_t PROPERTY(revenue_yesterday);
		fixed_point_t PROPERTY(output_quantity_yesterday);
		fixed_point_t PROPERTY(unsold_quantity_yesterday);
		fixed_point_t PROPERTY_RW(size_multiplier);
		memory::vector<Employee> PROPERTY(employees);
		pop_size_t PROPERTY(max_employee_count_cache, 0);
		pop_size_t PROPERTY(total_employees_count_cache, 0);
		pop_size_t PROPERTY(total_paid_employees_count_cache, 0);
		fixed_point_t PROPERTY(total_owner_income_cache);
		fixed_point_t PROPERTY(total_employee_income_cache);
		IndexedFlatMap_PROPERTY(PopType, pop_size_t, employee_count_per_type_cache);

		fixed_point_t calculate_size_modifier() const;
		void hire();
		fixed_point_t produce();
		void pay_employees(memory::vector<fixed_point_t>& reusable_vector);
		static void after_sell(void* actor, SellResult const& sell_result, memory::vector<fixed_point_t>& reusable_vector);

	public:
		ResourceGatheringOperation(
			MarketInstance& new_market_instance,
			ProductionType const* new_production_type_nullable,
			fixed_point_t new_size_multiplier,
			fixed_point_t new_revenue_yesterday,
			fixed_point_t new_output_quantity_yesterday,
			fixed_point_t new_unsold_quantity_yesterday,
			memory::vector<Employee>&& new_employees,
			decltype(employee_count_per_type_cache)::keys_span_type pop_type_keys
		);

		ResourceGatheringOperation(
			MarketInstance& new_market_instance,
			decltype(employee_count_per_type_cache)::keys_span_type pop_type_keys
		);

		constexpr bool is_valid() const {
			return production_type_nullable != nullptr;
		}
		void setup_location_ptr(ProvinceInstance& location);
		void initialise_rgo_size_multiplier();
		static constexpr size_t VECTORS_FOR_RGO_TICK = 1;
		void rgo_tick(memory::vector<fixed_point_t>& reusable_vector);
	};
}
#undef IndexedFlatMap_PROPERTY
#undef IndexedFlatMap_PROPERTY_ACCESS