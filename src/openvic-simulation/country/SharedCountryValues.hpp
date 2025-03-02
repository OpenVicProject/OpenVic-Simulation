#pragma once

#include "openvic-simulation/pop/PopNeedsMacro.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct CountryDefines;
	struct GoodInstanceManager;
	struct ModifierEffectCache;
	struct PopsDefines;
	struct PopType;

	struct SharedPopTypeValues {
	private:
		#define NEED_COST_FIELD(need_category) \
			fixed_point_t base_##need_category##_need_costs;
		DO_FOR_ALL_NEED_CATEGORIES(NEED_COST_FIELD)
		#undef NEED_COST_FIELD

		fixed_point_t PROPERTY(administration_salary_base);
		fixed_point_t PROPERTY(education_salary_base);
		fixed_point_t PROPERTY(military_salary_base);

	public:
		void update_costs(PopType const& pop_type, PopsDefines const& pop_defines, GoodInstanceManager const& good_instance_manager);
		constexpr fixed_point_t get_social_income_variant_base() const {
			return 2 * base_life_need_costs;
		}
	};

	struct SharedCountryValues {
	private:
		ModifierEffectCache const& PROPERTY(modifier_effect_cache);
		CountryDefines const& PROPERTY(country_defines);
		PopsDefines const& pop_defines;
		IndexedMap<PopType, SharedPopTypeValues> PROPERTY(shared_pop_type_values);

	public:
		SharedCountryValues(
			ModifierEffectCache const& new_modifier_effect_cache,
			CountryDefines const& new_country_defines,
			PopsDefines const& new_pop_defines,
			decltype(shared_pop_type_values)::keys_type const& pop_type_keys
		);
		SharedCountryValues(SharedCountryValues&&) = default;

		void update_costs(GoodInstanceManager const& good_instance_manager);
	};
}

#undef DO_FOR_ALL_NEED_CATEGORIES