#pragma once

#include "openvic-simulation/pop/PopNeedsMacro.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GoodInstanceManager;
	struct ModifierEffectCache;
	struct PopsDefines;
	struct PopType;

	struct SharedPopTypeValues {
	private:
		#define NEED_COST_PROPERTY(need_category) \
			fixed_point_t PROPERTY(base_##need_category##_need_costs);
		DO_FOR_ALL_NEED_CATEGORIES(NEED_COST_PROPERTY)
		#undef NEED_COST_PROPERTY

	public:
		void update_costs(PopType const& pop_type, GoodInstanceManager const& good_instance_manager);
	};

	struct SharedCountryValues {
	private:
		ModifierEffectCache const& PROPERTY(modifier_effect_cache);
		PopsDefines const& PROPERTY(pop_defines);
		IndexedMap<PopType, SharedPopTypeValues> PROPERTY(shared_pop_type_values);

	public:
		SharedCountryValues(
			ModifierEffectCache const& new_modifier_effect_cache,
			PopsDefines const& new_pop_defines,
			decltype(shared_pop_type_values)::keys_type const& pop_type_keys
		);

		void update_costs(GoodInstanceManager const& good_instance_manager);
	};
}

#undef DO_FOR_ALL_NEED_CATEGORIES