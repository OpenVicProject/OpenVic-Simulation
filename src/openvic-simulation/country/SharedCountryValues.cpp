#include "SharedCountryValues.hpp"

#include "openvic-simulation/defines/PopsDefines.hpp"
#include "openvic-simulation/economy/BuildingType.hpp" // IWYU pragma: keep for BuildingType size
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/GoodInstanceManager.hpp"
#include "openvic-simulation/military/UnitType.hpp" // IWYU pragma: keep for RegimentType size
#include "openvic-simulation/politics/Reform.hpp" // IWYU pragma: keep for Reform size
#include "openvic-simulation/population/PopNeedsMacro.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/research/Invention.hpp" // IWYU pragma: keep for Invention size
#include "openvic-simulation/research/Technology.hpp" // IWYU pragma: keep for Technology size
#include "openvic-simulation/types/TypedIndices.hpp"

using namespace OpenVic;

SharedCountryValues::SharedCountryValues(SharedCountryValuesDeps const& deps)
  : _shared_pop_type_values {
		deps.pop_types.size(),
		[pop_types = deps.pop_types](pop_type_index_t pop_type_index)->PopType const& {
			return pop_types[pop_type_index];
		}
	},
	good_instance_manager { deps.good_instance_manager },
	pop_defines { deps.pop_defines },
	building_types { deps.building_types },
	inventions { deps.inventions },
	pop_types { deps.pop_types },
	reforms { deps.reforms },
	regiment_types { deps.regiment_types },
	technologies { deps.technologies },
	shared_pop_type_values { _shared_pop_type_values } {}

void SharedCountryValues::update_costs() {
	for (SharedPopTypeValues& value : shared_pop_type_values) {
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
		for (auto const& [good_index, quantity] : pop_type.get_##need_category##_needs()) { \
			GoodInstance const& good_instance = *good_instance_manager.get_good_instance_by_index(good_index); \
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

	OV_DO_FOR_ALL_NEED_CATEGORIES(UPDATE_NEED_COSTS)

	#undef UPDATE_NEED_COSTS

	administration_salary_base.set(administration_salary_base_running_total);
	education_salary_base.set(education_salary_base_running_total);
	military_salary_base.set(military_salary_base_running_total);
	social_income_variant_base.set(2 * base_life_need_costs);
}
