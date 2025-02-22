#pragma once

#include <array>

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
		void update_pop_strata_values_from_province(PopsDefines const& defines, Strata const& strata, ProvinceInstance const& province);
	};

	struct PopValuesFromProvince {
	private:
		static constexpr size_t MAPS_FOR_POP = 4;
		PopsDefines const& PROPERTY(defines);
		IndexedMap<Strata, PopStrataValuesFromProvince> PROPERTY(effects_per_strata);
	public:
	 	//public field as mutable references are required.
		std::array<GoodDefinition::good_definition_map_t, MAPS_FOR_POP> reusable_maps {};

		PopValuesFromProvince(
			PopsDefines const& new_defines,
			decltype(effects_per_strata)::keys_type const& strata_keys
		);
		PopValuesFromProvince(PopValuesFromProvince&&) = default;

		void update_pop_values_from_province(ProvinceInstance const& province);
	};
}