#pragma once

#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"

namespace OpenVic {
	struct GoodDefinitionManager;

	struct GoodInstanceManager {
	private:
		GoodDefinitionManager const& good_definition_manager;
		OV_IFLATMAP_PROPERTY(GoodDefinition, GoodInstance, good_instance_by_definition);

	public:
		GoodInstanceManager(
			GoodDefinitionManager const& new_good_definition_manager,
			GameRulesManager const& game_rules_manager
		);

		constexpr forwardable_span<GoodInstance> get_good_instances() {
			return good_instance_by_definition.get_values();
		}

		constexpr forwardable_span<const GoodInstance> get_good_instances() const {
			return good_instance_by_definition.get_values();
		}
		GoodInstance* get_good_instance_by_identifier(std::string_view identifier);
		GoodInstance const* get_good_instance_by_identifier(std::string_view identifier) const;
		GoodInstance* get_good_instance_by_index(typename GoodInstance::index_t index);
		GoodInstance const* get_good_instance_by_index(typename GoodInstance::index_t index) const;
		GoodInstance& get_good_instance_by_definition(GoodDefinition const& good_definition);

		void enable_good(GoodDefinition const& good);
	};
}
