#pragma once

#include <functional>
#include <optional>
#include <string_view>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitBranchedGetterMacro.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct ProvinceDefinition;

	template<unit_branch_t>
	struct UnitDeployment;

	template<>
	struct UnitDeployment<unit_branch_t::LAND> {
		friend struct DeploymentManager;

	private:
		memory::string PROPERTY(name);
		ProvinceDefinition const* PROPERTY(home);

	public:
		RegimentType const& type;

		UnitDeployment(std::string_view new_name, RegimentType const& new_type, ProvinceDefinition const* new_home);
		UnitDeployment(UnitDeployment&&) = default;
	};

	using RegimentDeployment = UnitDeployment<unit_branch_t::LAND>;

	template<>
	struct UnitDeployment<unit_branch_t::NAVAL> {
		friend struct DeploymentManager;

	private:
		memory::string PROPERTY(name);

	public:
		ShipType const& type;

		UnitDeployment(std::string_view new_name, ShipType const& new_type);
		UnitDeployment(UnitDeployment&&) = default;
	};

	using ShipDeployment = UnitDeployment<unit_branch_t::NAVAL>;

	template<unit_branch_t Branch>
	struct UnitDeploymentGroup {
		friend struct DeploymentManager;

		using _Unit = UnitDeployment<Branch>;

	private:
		memory::string PROPERTY(name);
		std::reference_wrapper<const ProvinceDefinition> PROPERTY(location);
		memory::vector<_Unit> SPAN_PROPERTY(units);
		std::optional<size_t> PROPERTY(leader_index);

	public:
		constexpr UnitDeploymentGroup(
			std::string_view new_name,
			ProvinceDefinition const& new_location,
			memory::vector<_Unit>&& new_units,
			std::optional<size_t> new_leader_index
		) : name { new_name },
			location { new_location },
			units { std::move(new_units) },
			leader_index { new_leader_index } {}
		UnitDeploymentGroup(UnitDeploymentGroup&&) = default;
	};

	using ArmyDeployment = UnitDeploymentGroup<unit_branch_t::LAND>;
	using NavyDeployment = UnitDeploymentGroup<unit_branch_t::NAVAL>;

	struct Deployment : HasIdentifier {
	private:
		memory::vector<ArmyDeployment> SPAN_PROPERTY(armies);
		memory::vector<NavyDeployment> SPAN_PROPERTY(navies);
		memory::vector<LeaderBase> SPAN_PROPERTY(leaders);

	public:
		Deployment(
			std::string_view new_path,
			memory::vector<ArmyDeployment>&& new_armies,
			memory::vector<NavyDeployment>&& new_navies,
			memory::vector<LeaderBase>&& new_leaders
		);
		Deployment(Deployment&&) = default;

		OV_UNIT_BRANCHED_GETTER_CONST(get_unit_deployment_groups, armies, navies);
	};
}
