#include "SoundEffect.hpp"

using namespace OpenVic;

SoundEffect::SoundEffect( //
	std::string_view new_identifier, std::filesystem::path&& new_file, fixed_point_t new_volume
) : HasIdentifier { new_identifier }, file { std::move(new_file) }, volume { new_volume } {}
