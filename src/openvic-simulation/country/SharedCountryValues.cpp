#include "SharedCountryValues.hpp"

#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/pop/PopNeedsMacro.hpp"
#include "openvic-simulation/pop/PopType.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"

using namespace OpenVic;

SharedCountryValues::SharedCountryValues(
	ModifierEffectCache const& new_modifier_effect_cache,
	PopsDefines const& new_pop_defines,
	decltype(shared_pop_type_values)::keys_type const& pop_type_keys
) : modifier_effect_cache { new_modifier_effect_cache },
	pop_defines { new_pop_defines },
	shared_pop_type_values { &pop_type_keys }
	{}

void SharedCountryValues::update_costs(GoodInstanceManager const& good_instance_manager) {
	for (auto [pop_type, value] : shared_pop_type_values) {
		value.update_costs(pop_type, good_instance_manager);
	}
}

void SharedPopTypeValues::update_costs(PopType const& pop_type, GoodInstanceManager const& good_instance_manager) {
	#define UPDATE_NEED_COSTS(need_category) \
		base_##need_category##_need_costs = fixed_point_t::_0(); \
		for (auto const& [good_definition_ptr, quantity] : pop_type.get_##need_category##_needs()) { \
			GoodInstance const& good_instance = good_instance_manager.get_good_instance_from_definition(*good_definition_ptr); \
			base_##need_category##_need_costs += good_instance.get_price() * quantity; \
		}

	DO_FOR_ALL_NEED_CATEGORIES(UPDATE_NEED_COSTS)
	#undef UPDATE_NEED_COSTS
}

#undef DO_FOR_ALL_NEED_CATEGORIES