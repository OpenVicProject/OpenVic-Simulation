#pragma once

#include <optional>

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"

#include <function2/function2.hpp>

namespace OpenVic {
	struct GameManager {
		using elapsed_msec_getter_func_t = fu2::function_base<true, true, fu2::capacity_none, false, false, uint64_t() const>;

	private:
		inline static elapsed_msec_getter_func_t get_elapsed_time_callback = nullptr;

		GameRulesManager PROPERTY(game_rules_manager);
		Dataloader PROPERTY(dataloader);
		DefinitionManager PROPERTY(definition_manager);
		std::optional<InstanceManager> instance_manager;

		InstanceManager::gamestate_updated_func_t gamestate_updated_callback;
		bool PROPERTY_CUSTOM_PREFIX(definitions_loaded, are);

	public:
		GameManager(
			InstanceManager::gamestate_updated_func_t new_gamestate_updated_callback,
			elapsed_msec_getter_func_t new_get_elapsed_time_callback
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

		static uint64_t get_elapsed_milliseconds();
	};
}
