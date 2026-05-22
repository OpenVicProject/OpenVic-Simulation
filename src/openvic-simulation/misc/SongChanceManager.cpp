#include "SongChanceManager.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

bool SongChanceManager::load_songs_file(ovdl::v2script::ast::Node const* root) {
	bool ret = true;

	ret &= expect_dictionary_reserve_length(
		song_chances,
		[this](std::string_view key, ovdl::v2script::ast::Node const* value) -> bool {
			using enum scope_type_t;

			if (key != "song") {
				spdlog::error_s("Invalid song declaration {}", key);
				return false;
			}
			std::string_view name {};
			ConditionalWeightFactorMul chance { COUNTRY, COUNTRY, NO_SCOPE };

			bool ret = expect_dictionary_keys(
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(name)),
				"chance", ONE_EXACTLY, chance.expect_conditional_weight()
			)(value);

			ret &= song_chances.emplace_item(
				name,
				song_chances.size(), name, std::move(chance)
			);
			return ret;
		}
	)(root);

	if (song_chances.size() == 0) {
		spdlog::error_s("No songs found in Songs.txt");
		return false;
	}

	return ret;
}

bool SongChanceManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	for (SongChance& songChance : song_chances.get_items()) {
		ret &= songChance.parse_scripts(definition_manager);
	}
	return ret;
}
