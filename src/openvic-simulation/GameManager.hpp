#pragma once

#include <optional>

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/gen/commit_info.gen.hpp"

namespace OpenVic {
	struct GameManager {
	private:
		GameRulesManager PROPERTY(game_rules_manager);
		Dataloader PROPERTY(dataloader);
		DefinitionManager PROPERTY(definition_manager);
		std::optional<InstanceManager> instance_manager;

		InstanceManager::gamestate_updated_func_t gamestate_updated_callback;
		bool PROPERTY_CUSTOM_PREFIX(definitions_loaded, are);

	public:
		GameManager(
			InstanceManager::gamestate_updated_func_t new_gamestate_updated_callback
		);

		inline constexpr InstanceManager* get_instance_manager() {
			return instance_manager ? &*instance_manager : nullptr;
		}

		inline constexpr InstanceManager const* get_instance_manager() const {
			return instance_manager ? &*instance_manager : nullptr;
		}

		bool set_roots(Dataloader::path_vector_t const& roots);

		bool load_definitions(Dataloader::localisation_callback_t localisation_callback);

		bool setup_instance(Bookmark const* bookmark);

		bool start_game_session();

		bool update_clock();

		static constexpr std::string_view get_commit_hash() {
			return SIM_COMMIT_HASH;
		}

		static constexpr uint64_t get_commit_timestamp() {
			return SIM_COMMIT_TIMESTAMP;
		}
	};
}
