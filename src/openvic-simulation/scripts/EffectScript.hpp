#pragma once

#include "openvic-simulation/scripts/Script.hpp"

namespace OpenVic {
	struct DefinitionManager;

	struct EffectScript final : Script<DefinitionManager const&> {
	protected:
		bool _parse_script(std::span<const ast::NodeCPtr> nodes, DefinitionManager const& definition_manager) override;
	};
}
