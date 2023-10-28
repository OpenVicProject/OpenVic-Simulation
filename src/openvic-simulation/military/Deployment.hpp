#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/military/LeaderTrait.hpp"
#include "openvic-simulation/military/Unit.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct Leader {
		std::string name;
		const Unit::type_t type;
		const Date date;
		LeaderTrait const* personality;
		LeaderTrait const* background;
		fixed_point_t prestige;
	};

	struct Regiment {
		std::string name;
		Unit const* type;
		Province const* home;
	};

	struct Ship {
		std::string name;
		Unit const* type;
	};

	struct Army {
		std::string name;
		Province const* location;
		std::vector<Regiment> regiments;
	};

	struct Navy {
		std::string name;
		Province const* location;
		std::vector<Ship> ships;
	};

	struct DeploymentManager;

	struct Deployment : HasIdentifier {
		friend struct DeploymentManager;

	private:
		const std::vector<Army> armies;
		const std::vector<Navy> navies;
		const std::vector<Leader> leaders;

		Deployment(std::string_view new_path, std::vector<Army>&& new_armies,
			std::vector<Navy>&& new_navies, std::vector<Leader>&& new_leaders);

	public:
		const std::vector<Army>& get_armies() const;
		const std::vector<Navy>& get_navies() const;
		const std::vector<Leader>& get_leaders() const;
	};

	struct DeploymentManager {
	private:
		IdentifierRegistry<Deployment> deployments;

	public:
		DeploymentManager();

		bool add_deployment(std::string_view path, std::vector<Army>&& armies,
			std::vector<Navy>&& navies, std::vector<Leader>&& leaders);
		IDENTIFIER_REGISTRY_ACCESSORS(deployment);

		bool load_oob_file(GameManager& game_manager, std::string_view path, ast::NodeCPtr root);
	};
} // namespace OpenVic
