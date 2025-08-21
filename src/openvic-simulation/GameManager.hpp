#pragma once

#include <optional>
#include <string_view>

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/dataloader/ModManager.hpp"
#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/gen/commit_info.gen.hpp"
#include "openvic-simulation/utility/ForwardableSpan.hpp"

namespace OpenVic {
	struct GameManager {
	private:
		GameRulesManager PROPERTY(game_rules_manager);
		Dataloader PROPERTY(dataloader);
		DefinitionManager PROPERTY(definition_manager);
		ModManager PROPERTY(mod_manager);
		std::optional<InstanceManager> instance_manager;

		InstanceManager::gamestate_updated_func_t gamestate_updated_callback;
		bool PROPERTY_CUSTOM_PREFIX(definitions_loaded, are);
		bool PROPERTY_CUSTOM_PREFIX(mod_descriptors_loaded, are);

		bool _get_mod_dependencies(Mod const* mod, memory::vector<Mod const*>& load_list);

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

		inline bool set_base_path(Dataloader::path_span_t base_path) {
			OV_ERR_FAIL_COND_V_MSG(base_path.size() > 1, false, "Too many base paths were provided, only one should be set.");
			OV_ERR_FAIL_COND_V_MSG(!dataloader.set_roots(base_path, {}), false, "Failed to set Dataloader's base path");
			return true;
		};

		bool load_mod_descriptors();

		bool load_mods(
			Dataloader::path_vector_t& roots,
			Dataloader::path_vector_t& replace_paths,
			utility::forwardable_span<const memory::string> requested_mods
		);

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
