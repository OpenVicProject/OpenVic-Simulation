#pragma once

#include <optional>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"

#include "openvic-simulation/ModifierCalculationTestToggle.hpp"

namespace OpenVic {
	struct GameManager {
	private:
		Dataloader PROPERTY(dataloader);
		DefinitionManager PROPERTY(definition_manager);
		std::optional<InstanceManager> instance_manager;
#if OV_MODIFIER_CALCULATION_TEST
		std::optional<InstanceManager> instance_manager_no_add;
#endif

		InstanceManager::gamestate_updated_func_t gamestate_updated_callback;
		SimulationClock::state_changed_function_t clock_state_changed_callback;
		bool PROPERTY_CUSTOM_PREFIX(definitions_loaded, are);

	public:
		GameManager(
			InstanceManager::gamestate_updated_func_t new_gamestate_updated_callback,
			SimulationClock::state_changed_function_t new_clock_state_changed_callback
		);

		inline constexpr InstanceManager* get_instance_manager() {
			return instance_manager ? &*instance_manager : nullptr;
		}

		inline constexpr InstanceManager const* get_instance_manager() const {
			return instance_manager ? &*instance_manager : nullptr;
		}

#if OV_MODIFIER_CALCULATION_TEST
		inline constexpr InstanceManager* get_instance_manager_no_add() {
			return instance_manager_no_add ? &*instance_manager_no_add : nullptr;
		}

		inline constexpr InstanceManager const* get_instance_manager_no_add() const {
			return instance_manager_no_add ? &*instance_manager_no_add : nullptr;
		}
#endif

		bool set_roots(Dataloader::path_vector_t const& roots);

		bool load_definitions(Dataloader::localisation_callback_t localisation_callback);

		bool setup_instance(Bookmark const* bookmark);

		bool start_game_session();

		bool update_clock();
	};
}
