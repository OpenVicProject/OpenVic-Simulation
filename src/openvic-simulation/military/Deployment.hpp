#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitType.hpp"

namespace OpenVic {
	struct RegimentDeployment {
		friend struct DeploymentManager;

	private:
		std::string PROPERTY(name);
		RegimentType const& PROPERTY(type);
		Province const* PROPERTY(home);

		RegimentDeployment(std::string_view new_name, RegimentType const& new_type, Province const* new_home);

	public:
		RegimentDeployment(RegimentDeployment&&) = default;
	};

	struct ShipDeployment {
		friend struct DeploymentManager;

	private:
		std::string PROPERTY(name);
		ShipType const& PROPERTY(type);

		ShipDeployment(std::string_view new_name, ShipType const& new_type);

	public:
		ShipDeployment(ShipDeployment&&) = default;
	};

	struct ArmyDeployment {
		friend struct DeploymentManager;

	private:
		std::string PROPERTY(name);
		Province const* PROPERTY(location);
		std::vector<RegimentDeployment> PROPERTY(regiments);

		ArmyDeployment(
			std::string_view new_name, Province const* new_location, std::vector<RegimentDeployment>&& new_regiments
		);

	public:
		ArmyDeployment(ArmyDeployment&&) = default;
	};

	struct NavyDeployment {
		friend struct DeploymentManager;

	private:
		std::string PROPERTY(name);
		Province const* PROPERTY(location);
		std::vector<ShipDeployment> PROPERTY(ships);

		NavyDeployment(std::string_view new_name, Province const* new_location, std::vector<ShipDeployment>&& new_ships);

	public:
		NavyDeployment(NavyDeployment&&) = default;
	};

	struct Deployment : HasIdentifier {
		friend struct DeploymentManager;

	private:
		std::vector<ArmyDeployment> PROPERTY(armies);
		std::vector<NavyDeployment> PROPERTY(navies);
		std::vector<Leader> PROPERTY(leaders);

		Deployment(
			std::string_view new_path, std::vector<ArmyDeployment>&& new_armies, std::vector<NavyDeployment>&& new_navies,
			std::vector<Leader>&& new_leaders
		);

	public:
		Deployment(Deployment&&) = default;
	};

	struct DeploymentManager {
	private:
		IdentifierRegistry<Deployment> IDENTIFIER_REGISTRY(deployment);
		string_set_t missing_oob_files;

	public:
		bool add_deployment(
			std::string_view path, std::vector<ArmyDeployment>&& armies, std::vector<NavyDeployment>&& navies,
			std::vector<Leader>&& leaders
		);

		bool load_oob_file(
			GameManager const& game_manager, Dataloader const& dataloader, std::string_view history_path,
			Deployment const*& deployment, bool fail_on_missing
		);

		size_t get_missing_oob_file_count() const;
	};
} // namespace OpenVic
