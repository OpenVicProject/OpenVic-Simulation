#pragma once

#include <filesystem>

#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	class Dataloader;

	struct SoundEffect : HasIdentifier {
	private:
		std::filesystem::path PROPERTY(file);
		fixed_point_t PROPERTY(volume);

	public:
		SoundEffect(std::string_view new_identifier, std::filesystem::path&& new_file, fixed_point_t new_volume);
		SoundEffect(SoundEffect&&) = default;
	};

	class SoundEffectManager {
		IdentifierRegistry<SoundEffect> IDENTIFIER_REGISTRY(sound_effect);
		bool _load_sound_define(Dataloader const& dataloader, std::string_view sfx_identifier, ast::NodeCPtr root);

	public:
		bool load_sound_defines_file(Dataloader const& dataloader, ast::NodeCPtr root);
	};
}
