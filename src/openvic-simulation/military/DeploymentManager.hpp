#pragma once

#include <string_view>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/military/Deployment.hpp"
#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitBranchedGetterMacro.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	class Dataloader;
	struct MapDefinition;
	struct MilitaryManager;

	struct DeploymentManager {
	private:
		IdentifierRegistry<Deployment> IDENTIFIER_REGISTRY(deployment);
		string_set_t missing_oob_files;

	public:
		bool add_deployment(
			std::string_view path,
			memory::vector<ArmyDeployment>&& armies,
			memory::vector<NavyDeployment>&& navies,
			memory::vector<LeaderBase>&& leaders
		);

		bool load_oob_file(
			Dataloader const& dataloader,
			MapDefinition const& map_definition,
			MilitaryManager const& military_manager,
			std::string_view history_path,
			Deployment const*& deployment,
			bool fail_on_missing
		);

		size_t get_missing_oob_file_count() const;
	};
}
