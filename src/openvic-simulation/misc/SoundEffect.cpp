#include "SoundEffect.hpp"

#include "openvic-simulation/dataloader/Dataloader.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

SoundEffect::SoundEffect( //
	std::string_view new_identifier, std::filesystem::path&& new_file, fixed_point_t new_volume
)
	: HasIdentifier { new_identifier }, file { std::move(new_file) }, volume { new_volume } {}

bool SoundEffectManager::_load_sound_define(Dataloader const& dataloader, std::string_view sfx_identifier, ast::NodeCPtr root) {
	std::filesystem::path file {};
	auto file_callback = [&dataloader, &file](std::string_view val) -> bool {
		memory::string lookup = StringUtils::append_string_views("sound/", val);
		file = dataloader.lookup_file(lookup, false);
		if (file.empty()) {
			spdlog::warn_s("Lookup for \"{}\" failed!", lookup);
		}
		return true;
	};

	fixed_point_t volume = 1;
	bool ret = expect_dictionary_keys(
		"file", ONE_EXACTLY, expect_string(file_callback), //
		"volume", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(volume)) //
	)(root);

	if (sfx_identifier.empty()) {
		spdlog::error_s("Invalid sound identifier - empty!");
		return false;
	}
	if (file.empty()) {
		spdlog::warn_s("Sound filename {} was empty!", sfx_identifier);
	}

	ret &= sound_effects.emplace_item(
		sfx_identifier,
		sfx_identifier, std::move(file), volume
	);
	return ret;
}

bool SoundEffectManager::load_sound_defines_file(Dataloader const& dataloader, ast::NodeCPtr root) {
	return expect_dictionary_reserve_length(sound_effects, //
		[this, &dataloader](std::string_view key, ast::NodeCPtr value) -> bool {
			return _load_sound_define(dataloader, key, value);
		}
	)(root);
}
