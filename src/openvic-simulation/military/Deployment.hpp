#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct ProvinceDefinition;

	template<UnitType::branch_t>
	struct UnitDeployment;

	template<>
	struct UnitDeployment<UnitType::branch_t::LAND> {
		friend struct DeploymentManager;

	private:
		std::string PROPERTY(name);
		RegimentType const& PROPERTY(type);
		ProvinceDefinition const* PROPERTY(home);

		UnitDeployment(std::string_view new_name, RegimentType const& new_type, ProvinceDefinition const* new_home);

	public:
		UnitDeployment(UnitDeployment&&) = default;
	};

	using RegimentDeployment = UnitDeployment<UnitType::branch_t::LAND>;

	template<>
	struct UnitDeployment<UnitType::branch_t::NAVAL> {
		friend struct DeploymentManager;

	private:
		std::string PROPERTY(name);
		ShipType const& PROPERTY(type);

		UnitDeployment(std::string_view new_name, ShipType const& new_type);

	public:
		UnitDeployment(UnitDeployment&&) = default;
	};

	using ShipDeployment = UnitDeployment<UnitType::branch_t::NAVAL>;

	template<UnitType::branch_t Branch>
	struct UnitDeploymentGroup {
		friend struct DeploymentManager;

		using _Unit = UnitDeployment<Branch>;

	private:
		std::string PROPERTY(name);
		ProvinceDefinition const* PROPERTY(location);
		std::vector<_Unit> PROPERTY(units);
		std::optional<size_t> PROPERTY(leader_index);

		UnitDeploymentGroup(
			std::string_view new_name, ProvinceDefinition const* new_location, std::vector<_Unit>&& new_units,
			std::optional<size_t> new_leader_index
		);

	public:
		UnitDeploymentGroup(UnitDeploymentGroup&&) = default;
	};

	using ArmyDeployment = UnitDeploymentGroup<UnitType::branch_t::LAND>;
	using NavyDeployment = UnitDeploymentGroup<UnitType::branch_t::NAVAL>;

	struct Deployment : HasIdentifier {
		friend struct DeploymentManager;

	private:
		std::vector<ArmyDeployment> PROPERTY(armies);
		std::vector<NavyDeployment> PROPERTY(navies);
		std::vector<LeaderBase> PROPERTY(leaders);

		Deployment(
			std::string_view new_path, std::vector<ArmyDeployment>&& new_armies, std::vector<NavyDeployment>&& new_navies,
			std::vector<LeaderBase>&& new_leaders
		);

	public:
		Deployment(Deployment&&) = default;

		UNIT_BRANCHED_GETTER_CONST(get_unit_deployment_groups, armies, navies);
	};

	struct DefinitionManager;
	class Dataloader;

	struct DeploymentManager {
	private:
		IdentifierRegistry<Deployment> IDENTIFIER_REGISTRY(deployment);
		string_set_t missing_oob_files;

	public:
		bool add_deployment(
			std::string_view path, std::vector<ArmyDeployment>&& armies, std::vector<NavyDeployment>&& navies,
			std::vector<LeaderBase>&& leaders
		);

		bool load_oob_file(
			DefinitionManager const& definition_manager, Dataloader const& dataloader, std::string_view history_path,
			Deployment const*& deployment, bool fail_on_missing
		);

		size_t get_missing_oob_file_count() const;
	};
}
