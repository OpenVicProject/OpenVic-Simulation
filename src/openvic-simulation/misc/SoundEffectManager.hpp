#pragma once

#include "openvic-simulation/misc/SoundEffect.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	class Dataloader;

	class SoundEffectManager {
		IdentifierRegistry<SoundEffect> IDENTIFIER_REGISTRY(sound_effect);
		bool _load_sound_define(Dataloader const& dataloader, std::string_view sfx_identifier, ovdl::v2script::ast::Node const* root);

	public:
		bool load_sound_defines_file(Dataloader const& dataloader, ovdl::v2script::ast::Node const* root);
	};
}
