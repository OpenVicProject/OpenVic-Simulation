#include "SongChance.hpp"

using namespace OpenVic;

SongChance::SongChance(
	size_t new_index, std::string_view new_filename, ConditionalWeightFactorMul&& new_chance
) : HasIdentifier { std::to_string(new_index) }, file_name { new_filename }, chance { std::move(new_chance) } {}

bool SongChance::parse_scripts(DefinitionManager const& definition_manager) {
	return chance.parse_scripts(definition_manager);
}
