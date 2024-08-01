#pragma once

#include <openvic-simulation/types/fixed_point/FixedPoint.hpp>
#include <openvic-simulation/types/IdentifierRegistry.hpp>

namespace OpenVic {
	/*For interface/Sound.sfx */
	struct SoundEffectManager;
	struct SoundEffect : HasIdentifier {
	private:
		friend struct SoundEffectManager;
		std::string PROPERTY(file);
		fixed_point_t PROPERTY(volume);
		SoundEffect(std::string_view new_identifier, std::string_view new_file, fixed_point_t new_volume);
	
	public:
		SoundEffect(SoundEffect&&) = default;
	};
	
	struct SoundEffectManager {
	
	private:
		IdentifierRegistry<SoundEffect> IDENTIFIER_REGISTRY(sound_effect);	
		bool _load_sound_define(std::string_view sfx_identifier, ast::NodeCPtr root);

	public:
		bool load_sound_defines_file(ast::NodeCPtr root);
	};
}