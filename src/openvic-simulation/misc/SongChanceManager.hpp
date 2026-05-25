#pragma once

#include "openvic-simulation/misc/SongChance.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct SongChanceManager {
	private:
		IdentifierRegistry<SongChance> IDENTIFIER_REGISTRY(song_chance);
		//Songs.txt
	public:
		bool load_songs_file(ovdl::v2script::ast::Node const* root);
		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
