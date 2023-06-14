#pragma once
#include <filesystem>
#include "../Simulation.hpp"

namespace OpenVic {
	class Dataloader {
		public:
		enum class LoadingMode {
			DL_COMPATABILITY
		};

		static bool loadDir(std::filesystem::path p, Simulation& sim, LoadingMode loadMode = LoadingMode::DL_COMPATABILITY) {
			return true;
		}
	};
}