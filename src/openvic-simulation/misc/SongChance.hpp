#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"

namespace OpenVic {
	/*For music/Songs.txt if it exists*/
	struct SongChanceManager;

	struct SongChance : HasIdentifier {
		friend struct SongChanceManager;

	private:
		memory::string PROPERTY(file_name);
		ConditionalWeightFactorMul PROPERTY(chance);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		SongChance(size_t new_index, std::string_view new_filename, ConditionalWeightFactorMul&& new_chance);
		SongChance(SongChance&&) = default;
	};

	struct SongChanceManager {
	private:
		IdentifierRegistry<SongChance> IDENTIFIER_REGISTRY(song_chance);
		//Songs.txt
	public:
		bool load_songs_file(ovdl::v2script::ast::Node const* root);
		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}

