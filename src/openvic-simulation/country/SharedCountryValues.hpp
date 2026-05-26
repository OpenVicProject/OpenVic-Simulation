#pragma once

#include "openvic-simulation/core/memory/FixedVector.hpp"
#include "openvic-simulation/core/stl/containers/TypedSpan.hpp"
#include "openvic-simulation/country/SharedCountryValuesDeps.hpp"
#include "openvic-simulation/population/PopNeedsMacro.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/utility/reactive/MutableState.hpp"

namespace OpenVic {
	struct GoodInstanceManager;
	struct PopsDefines;
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

	struct CountryInstanceManager;
	struct SharedCountryValues {
		friend CountryInstanceManager;
	private:
		GoodInstanceManager const& good_instance_manager;
		PopsDefines const& pop_defines;
		memory::FixedVector<SharedPopTypeValues, pop_type_index_t> _shared_pop_type_values;

		void update_costs();

	public:
		TypedSpan<building_type_index_t, const BuildingType> building_types;
		TypedSpan<invention_index_t, const Invention> inventions;
		TypedSpan<pop_type_index_t, const PopType> pop_types;
		TypedSpan<reform_index_t, const Reform> reforms;
		TypedSpan<regiment_type_index_t, const RegimentType> regiment_types;
		TypedSpan<technology_index_t, const Technology> technologies;

		// mutable SharedPopTypeValues required
		TypedSpan<pop_type_index_t, SharedPopTypeValues> shared_pop_type_values;

		SharedCountryValues(SharedCountryValuesDeps const& deps);
		SharedCountryValues(SharedCountryValues&&) = delete;
		SharedCountryValues(SharedCountryValues const&) = delete;
		SharedCountryValues& operator=(SharedCountryValues&&) = delete;
		SharedCountryValues& operator=(SharedCountryValues const&) = delete;
	};
}
