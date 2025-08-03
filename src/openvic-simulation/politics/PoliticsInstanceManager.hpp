#pragma once

#include <optional>

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IndexedFlatMapMacro.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct InstanceManager;
	struct PoliticsManager;
	struct Ideology;

	struct PoliticsInstanceManager {
	private:
		InstanceManager const& PROPERTY(instance_manager);
		PoliticsManager const& PROPERTY(politics_manager);

		IndexedFlatMap_PROPERTY(Ideology, std::optional<Date>, ideology_spawn_date);

		bool PROPERTY(great_wars_enabled, false);
		bool PROPERTY(world_wars_enabled, false);

	public:
		PoliticsInstanceManager(InstanceManager const& new_instance_manager);
		PoliticsInstanceManager(PoliticsInstanceManager&&) = default;

		void setup_starting_ideologies();

		bool is_ideology_unlocked(Ideology const& ideology) const;
		void unlock_ideology(Ideology const& ideology);

		// Enabling world wars automatically enables great wars too, although the same doesn't apply to disabling them
		void set_great_wars_enabled(bool enabled);
		void set_world_wars_enabled(bool enabled);
	};
}
#undef IndexedFlatMap_PROPERTY
#undef IndexedFlatMap_PROPERTY_ACCESS