#pragma once

#include <string>
#include <string_view>

#include "openvic-simulation/military/LeaderTrait.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	struct DeploymentManager;

	struct LeaderBase {
		friend struct DeploymentManager;

	private:
		std::string        PROPERTY(name);
		UnitType::branch_t PROPERTY(branch); /* type in defines */
		Date               PROPERTY(date);
		LeaderTrait const* PROPERTY(personality);
		LeaderTrait const* PROPERTY(background);
		fixed_point_t      PROPERTY(prestige);
		std::string        PROPERTY(picture);

	private:
		LeaderBase(
			std::string_view new_name, UnitType::branch_t new_branch, Date new_date, LeaderTrait const* new_personality,
			LeaderTrait const* new_background, fixed_point_t new_prestige, std::string_view new_picture
		);

	protected:
		LeaderBase(LeaderBase const&) = default;

	public:
		LeaderBase(LeaderBase&&) = default;
	};

	struct UnitInstanceManager;

	template<UnitType::branch_t>
	struct UnitInstanceGroup;

	template<UnitType::branch_t>
	struct UnitInstanceGroupBranched;

	template<UnitType::branch_t Branch>
	struct LeaderBranched : LeaderBase {

		friend struct UnitInstanceManager;
		friend bool UnitInstanceGroup<Branch>::set_leader(LeaderBranched<Branch>* new_leader);

	private:
		UnitInstanceGroupBranched<Branch>* PROPERTY(unit_instance_group);

		LeaderBranched(LeaderBase const& leader_base) : LeaderBase { leader_base }, unit_instance_group { nullptr } {}
	};

	using General = LeaderBranched<UnitType::branch_t::LAND>;
	using Admiral = LeaderBranched<UnitType::branch_t::NAVAL>;
}
