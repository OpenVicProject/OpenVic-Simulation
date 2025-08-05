#pragma once

#include "openvic-simulation/economy/trading/GoodMarket.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IndexedFlatMapMacro.hpp"

namespace OpenVic {
	struct GoodDefinition;
	struct GoodDefinitionManager;
	struct GoodInstanceManager;

	struct GoodInstance : HasIdentifierAndColour, HasIndex<GoodInstance>, GoodMarket {
		friend struct GoodInstanceManager;

	public:
		//pointers instead of references to allow construction via std::tuple
		GoodInstance(
			GoodDefinition const* new_good_definition,
			GameRulesManager const* new_game_rules_manager
		);
		GoodInstance(GoodInstance const&) = delete;
		GoodInstance& operator=(GoodInstance const&) = delete;
		GoodInstance(GoodInstance&&) = delete;
		GoodInstance& operator=(GoodInstance&&) = delete;

		// Is the good available for trading? (e.g. should be shown in trade menu)
		// is_tradeable has no effect on this, only is_money and availability
		bool is_trading_good() const;
	};

	struct GoodInstanceManager {
	private:
		GoodDefinitionManager const& PROPERTY(good_definition_manager);
		IndexedFlatMap_PROPERTY(GoodDefinition, GoodInstance, good_instance_by_definition);

	public:
		GoodInstanceManager(
			GoodDefinitionManager const& new_good_definition_manager,
			GameRulesManager const& game_rules_manager
		);

		constexpr OpenVic::utility::forwardable_span<GoodInstance> get_good_instances() {
			return good_instance_by_definition.get_values();
		}

		constexpr OpenVic::utility::forwardable_span<const GoodInstance> get_good_instances() const {
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
