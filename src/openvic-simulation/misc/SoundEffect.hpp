#pragma once

#include <filesystem>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"

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
}
