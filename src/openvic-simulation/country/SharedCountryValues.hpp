#pragma once

#include "openvic-simulation/population/PopNeedsMacro.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/reactive/MutableState.hpp"

namespace OpenVic {
	struct CountryInstanceManager;
	struct GoodInstanceManager;
	struct PopsDefines;
	struct PopType;
	struct SharedCountryValues;

	struct SharedPopTypeValues {
		friend SharedCountryValues;
	private:
		PopType const& pop_type;

		#define NEED_COST_FIELD(need_category) \
			fixed_point_t base_##need_category##_need_costs;

		OV_DO_FOR_ALL_NEED_CATEGORIES(NEED_COST_FIELD)

		#undef NEED_COST_FIELD

		OV_STATE_PROPERTY(fixed_point_t, administration_salary_base);
		OV_STATE_PROPERTY(fixed_point_t, education_salary_base);
		OV_STATE_PROPERTY(fixed_point_t, military_salary_base);
		OV_STATE_PROPERTY(fixed_point_t, social_income_variant_base);

		void update_costs(PopsDefines const& pop_defines, GoodInstanceManager const& good_instance_manager);
	public:
		constexpr SharedPopTypeValues(PopType const& new_pop_type) : pop_type { new_pop_type } {};
	};

	struct SharedCountryValues {
		friend CountryInstanceManager;
	private:
		PopsDefines const& pop_defines;
		GoodInstanceManager const& good_instance_manager;
		IndexedFlatMap<PopType, SharedPopTypeValues> shared_pop_type_values;

		void update_costs();

	public:
		SharedPopTypeValues& get_shared_pop_type_values(PopType const& pop_type);

		SharedCountryValues(
			PopsDefines const& new_pop_defines,
			GoodInstanceManager const& new_good_instance_manager,
			decltype(shared_pop_type_values)::keys_span_type pop_type_keys
		);
		SharedCountryValues(SharedCountryValues&&) = delete;
		SharedCountryValues(SharedCountryValues const&) = delete;
		SharedCountryValues& operator=(SharedCountryValues&&) = delete;
		SharedCountryValues& operator=(SharedCountryValues const&) = delete;
	};
}
