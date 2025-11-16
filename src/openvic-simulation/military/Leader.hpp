#pragma once

#include <string_view>

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/types/UniqueId.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct DeploymentManager;
	struct UnitInstanceManager;
	struct LeaderTrait;

	struct LeaderBase {
		friend struct DeploymentManager;
		friend struct UnitInstanceManager;

	private:
		memory::string PROPERTY(name);
		LeaderTrait const* PROPERTY(personality);
		LeaderTrait const* PROPERTY(background);
		fixed_point_t PROPERTY(prestige);
		memory::string PROPERTY(picture);

	protected:
		LeaderBase(LeaderBase const&) = default;

	public:
		const unit_branch_t branch; /* type in defines */
		const Date date;

		LeaderBase(
			std::string_view new_name, unit_branch_t new_branch, Date new_date, LeaderTrait const* new_personality,
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
		UnitInstanceGroup* PROPERTY_PTR(unit_instance_group, nullptr);
		bool PROPERTY_RW(can_be_used, true);

		LeaderInstance(unique_id_t new_unique_id, LeaderBase const& leader_base, CountryInstance const& new_country);
	public:
		const unique_id_t unique_id;

		CountryInstance const& country;
	};
}
