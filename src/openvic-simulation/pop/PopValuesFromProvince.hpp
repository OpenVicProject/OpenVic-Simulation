#pragma once

#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct ProvinceInstance;
	struct PopsDefines;
	struct PopValuesFromProvince;
	struct Strata;

	struct PopStrataValuesFromProvince {
	private:
		fixed_point_t PROPERTY(shared_life_needs_scalar);
		fixed_point_t PROPERTY(shared_everyday_needs_scalar);
		fixed_point_t PROPERTY(shared_luxury_needs_scalar);
	public:
		void update_pop_strata_values_from_province(
			PopsDefines const& defines,
			Strata const& strata,
			ProvinceInstance const& province
		);
	};

	struct PopValuesFromProvince {
	private:
		PopsDefines const& PROPERTY(defines);
		OV_IFLATMAP_PROPERTY(Strata, PopStrataValuesFromProvince, effects_by_strata);
	public:
		PopValuesFromProvince(
			PopsDefines const& new_defines,
			decltype(effects_by_strata)::keys_span_type strata_keys
		);
		PopValuesFromProvince(PopValuesFromProvince&&) = default;

		void update_pop_values_from_province(ProvinceInstance const& province);
	};
}