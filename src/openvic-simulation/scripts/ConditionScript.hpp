#pragma once

#include "openvic-simulation/scripts/Script.hpp"

namespace OpenVic {
	struct GameManager;

	struct ConditionScript final : Script<GameManager const&> {
	protected:
		bool _parse_script(ast::NodeCPtr root, GameManager const& game_manager) override;
	};
}
