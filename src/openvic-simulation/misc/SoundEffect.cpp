#include "SoundEffect.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

SoundEffect::SoundEffect (
	std::string_view new_identifier, std::string_view new_file, fixed_point_t new_volume
) : HasIdentifier { new_identifier }, file { new_file }, volume { new_volume } {}

bool SoundEffectManager::_load_sound_define(std::string_view sfx_identifier, ast::NodeCPtr root) {
	std::string_view file {};
	fixed_point_t volume = 1;
	bool ret = expect_dictionary_keys(
		"file", ONE_EXACTLY, expect_string(assign_variable_callback(file)),
		"volume", ZERO_OR_ONE,
			expect_fixed_point(assign_variable_callback(volume))
	)(root);

	if (sfx_identifier.empty()) {
		Logger::error("Invalid sound identifier - empty!");
		return false;
	}
	if(file.empty()) {
		Logger::error("Invalid sound file name - empty!");
		return false;
	}

	ret &= sound_effects.add_item({sfx_identifier,file,volume});
	return ret;
}

bool SoundEffectManager::load_sound_defines_file(ast::NodeCPtr root) {
	return expect_dictionary_reserve_length(sound_effects,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			return _load_sound_define(key,value);
		}
	)(root);
}