#pragma once

#include "openvic-simulation/scripts/Script.hpp"

namespace OpenVic {
	struct GameManager;

	struct EffectScript final : Script<GameManager&> {
	protected:
		bool _parse_script(ast::NodeCPtr root, GameManager& game_manager) override;
	};
}
