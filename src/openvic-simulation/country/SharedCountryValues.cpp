#include "SharedCountryValues.hpp"

#include "openvic-simulation/defines/PopsDefines.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/pop/PopNeedsMacro.hpp"
#include "openvic-simulation/pop/PopType.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"

using namespace OpenVic;

SharedCountryValues::SharedCountryValues(
	ModifierEffectCache const& new_modifier_effect_cache,
	CountryDefines const& new_country_defines,
	PopsDefines const& new_pop_defines,
	decltype(shared_pop_type_values)::keys_span_type pop_type_keys
) : modifier_effect_cache { new_modifier_effect_cache },
	country_defines { new_country_defines },
	pop_defines { new_pop_defines },
	shared_pop_type_values { pop_type_keys }
	{}

void SharedCountryValues::update_costs(GoodInstanceManager const& good_instance_manager) {
	for (auto [pop_type, value] : shared_pop_type_values) {
		value.update_costs(pop_type, pop_defines, good_instance_manager);
	}
}

void SharedPopTypeValues::update_costs(PopType const& pop_type, PopsDefines const& pop_defines, GoodInstanceManager const& good_instance_manager) {
	administration_salary_base = education_salary_base = military_salary_base = fixed_point_t::_0();
	using enum PopType::income_type_t;

	#define UPDATE_NEED_COSTS(need_category) \
		base_##need_category##_need_costs = fixed_point_t::_0(); \
		for (auto const& [good_definition_ptr, quantity] : pop_type.get_##need_category##_needs()) { \
			GoodInstance const& good_instance = good_instance_manager.get_good_instance_from_definition(*good_definition_ptr); \
			base_##need_category##_need_costs += good_instance.get_price() * quantity; \
		} \
		base_##need_category##_need_costs *= pop_defines.get_base_goods_demand(); \
		if ((pop_type.get_##need_category##_needs_income_types() & ADMINISTRATION) == ADMINISTRATION) { \
			administration_salary_base += base_##need_category##_need_costs; \
		} \
		if ((pop_type.get_##need_category##_needs_income_types() & EDUCATION) == EDUCATION) { \
			education_salary_base += base_##need_category##_need_costs; \
		} \
		if ((pop_type.get_##need_category##_needs_income_types() & MILITARY) == MILITARY) { \
			military_salary_base += base_##need_category##_need_costs; \
		}

	DO_FOR_ALL_NEED_CATEGORIES(UPDATE_NEED_COSTS)
	#undef UPDATE_NEED_COSTS
}

#undef DO_FOR_ALL_NEED_CATEGORIES