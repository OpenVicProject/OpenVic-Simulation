#include "SharedCountryValues.hpp"

#include "openvic-simulation/defines/PopsDefines.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/pop/PopNeedsMacro.hpp"
#include "openvic-simulation/pop/PopType.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"

using namespace OpenVic;

SharedCountryValues::SharedCountryValues(
	PopsDefines const& new_pop_defines,
	GoodInstanceManager const& new_good_instance_manager,
	decltype(shared_pop_type_values)::keys_span_type pop_type_keys
) : pop_defines { new_pop_defines },
	good_instance_manager { new_good_instance_manager },
	shared_pop_type_values { pop_type_keys }
	{}

SharedPopTypeValues& SharedCountryValues::get_shared_pop_type_values(PopType const& pop_type) {
	return shared_pop_type_values.at(pop_type);
}

void SharedCountryValues::update_costs() {
	for (SharedPopTypeValues& value : shared_pop_type_values.get_values()) {
		value.update_costs(pop_defines, good_instance_manager);
	}
}

void SharedPopTypeValues::update_costs(PopsDefines const& pop_defines, GoodInstanceManager const& good_instance_manager) {
	fixed_point_t administration_salary_base_running_total = 0;
	fixed_point_t education_salary_base_running_total = 0;
	fixed_point_t military_salary_base_running_total = 0;
	using enum PopType::income_type_t;

	#define UPDATE_NEED_COSTS(need_category) \
		base_##need_category##_need_costs = 0; \
		for (auto const& [good_definition_ptr, quantity] : pop_type.get_##need_category##_needs()) { \
			GoodInstance const& good_instance = good_instance_manager.get_good_instance_by_definition(*good_definition_ptr); \
			base_##need_category##_need_costs += good_instance.get_price() * quantity; \
		} \
		base_##need_category##_need_costs *= pop_defines.get_base_goods_demand(); \
		if ((pop_type.get_##need_category##_needs_income_types() & ADMINISTRATION) == ADMINISTRATION) { \
			administration_salary_base_running_total += base_##need_category##_need_costs; \
		} \
		if ((pop_type.get_##need_category##_needs_income_types() & EDUCATION) == EDUCATION) { \
			education_salary_base_running_total += base_##need_category##_need_costs; \
		} \
		if ((pop_type.get_##need_category##_needs_income_types() & MILITARY) == MILITARY) { \
			military_salary_base_running_total += base_##need_category##_need_costs; \
		}

	DO_FOR_ALL_NEED_CATEGORIES(UPDATE_NEED_COSTS)
	#undef UPDATE_NEED_COSTS

	administration_salary_base.set(administration_salary_base_running_total);
	education_salary_base.set(education_salary_base_running_total);
	military_salary_base.set(military_salary_base_running_total);
	social_income_variant_base.set(2 * base_life_need_costs);
}

#undef DO_FOR_ALL_NEED_CATEGORIES