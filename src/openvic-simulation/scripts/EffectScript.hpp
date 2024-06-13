#pragma once

#include "openvic-simulation/scripts/Script.hpp"

namespace OpenVic {
	struct DefinitionManager;

	struct EffectScript final : Script<DefinitionManager const&> {
	protected:
		bool _parse_script(ast::NodeCPtr root, DefinitionManager const& definition_manager) override;
	};
}
