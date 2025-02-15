#pragma once

#include "openvic-simulation/pop/PopType.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct ProvinceInstance;
	struct PopsDefines;
	struct PopValuesFromProvince;

	struct PopStrataValuesFromProvince {
	private:
		fixed_point_t PROPERTY(shared_life_needs_scalar);
		fixed_point_t PROPERTY(shared_everyday_needs_scalar);
		fixed_point_t PROPERTY(shared_luxury_needs_scalar);
	public:
		void update_pop_strata_values_from_province(PopValuesFromProvince const& parent, Strata const& strata);
	};

	struct PopValuesFromProvince {
	private:
		PopsDefines const& PROPERTY(defines);
		ProvinceInstance const* PROPERTY_RW(province, nullptr);
		IndexedMap<Strata, PopStrataValuesFromProvince> PROPERTY(effects_per_strata);
	public:
		PopValuesFromProvince(
			PopsDefines const& new_defines,
			decltype(effects_per_strata)::keys_type const& strata_keys
		);

		void update_pop_values_from_province();
	};
}