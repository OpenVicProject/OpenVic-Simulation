#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	using unique_id_t = uint64_t;

	struct DeploymentManager;
	struct UnitInstanceManager;
	struct LeaderTrait;

	struct LeaderBase {
		friend struct DeploymentManager;
		friend struct UnitInstanceManager;

	private:
		std::string PROPERTY(name);
		const UnitType::branch_t PROPERTY(branch); /* type in defines */
		const Date PROPERTY(date);
		LeaderTrait const* PROPERTY(personality);
		LeaderTrait const* PROPERTY(background);
		fixed_point_t PROPERTY(prestige);
		std::string PROPERTY(picture);

	protected:
		LeaderBase(LeaderBase const&) = default;

	public:
		LeaderBase(
			std::string_view new_name, UnitType::branch_t new_branch, Date new_date, LeaderTrait const* new_personality,
			LeaderTrait const* new_background, fixed_point_t new_prestige, std::string_view new_picture
		);
		LeaderBase(LeaderBase&&) = default;

		void set_picture(std::string_view new_picture);
	};

	struct UnitInstanceManager;
	struct UnitInstanceGroup;
	struct CountryInstance;

	struct LeaderInstance : LeaderBase {

		friend struct UnitInstanceManager;
		friend struct UnitInstanceGroup;

	private:
		const unique_id_t PROPERTY(unique_id);
		CountryInstance const& PROPERTY(country);
		UnitInstanceGroup* PROPERTY_PTR(unit_instance_group, nullptr);
		bool PROPERTY_RW(can_be_used, true);

		LeaderInstance(unique_id_t new_unique_id, LeaderBase const& leader_base, CountryInstance const& new_country);
	};
}
