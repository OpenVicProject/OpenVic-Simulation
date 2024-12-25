#pragma once

#include <optional>

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct InstanceManager;
	struct PoliticsManager;
	struct Ideology;

	struct PoliticsInstanceManager {
	private:
		InstanceManager const& PROPERTY(instance_manager);
		PoliticsManager const& PROPERTY(politics_manager);

		IndexedMap<Ideology, std::optional<Date>> PROPERTY(ideology_spawn_date);

	public:
		PoliticsInstanceManager(InstanceManager const& new_instance_manager);
		PoliticsInstanceManager(PoliticsInstanceManager&&) = default;

		std::optional<Date> const& get_ideology_spawn_date(Ideology const& ideology) const;
		bool is_ideology_unlocked(Ideology const& ideology) const;
		void unlock_ideology(Ideology const& ideology);
	};
}
