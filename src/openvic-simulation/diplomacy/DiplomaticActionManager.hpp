#pragma once

#include <any>
#include <string_view>

#include <function2/function2.hpp>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/diplomacy/DiplomaticAction.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct InstanceManager;

	struct DiplomaticActionManager {
	private:
		IdentifierRegistry<DiplomaticActionTypeStorage> IDENTIFIER_REGISTRY(diplomatic_action_type);

	public:
		constexpr DiplomaticActionManager() {};

		bool add_diplomatic_action(std::string_view identifier, DiplomaticActionType::Initializer&& initializer);
		bool add_cancelable_diplomatic_action(
			std::string_view identifier, CancelableDiplomaticActionType::Initializer&& initializer
		);

		DiplomaticActionTickCache create_diplomatic_action_tick(
			std::string_view identifier, CountryInstance* sender, CountryInstance* receiver, std::any context_data,
			InstanceManager& instance_manager
		);

		bool setup_diplomatic_actions();
	};
}
