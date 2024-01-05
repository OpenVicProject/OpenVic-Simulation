#include "DiplomaticAction.hpp"

#include <string_view>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

DiplomaticActionType::DiplomaticActionType(DiplomaticActionType::Initializer&& initializer)
	: commit_action_caller { std::move(initializer.commit) }, allowed_to_commit { std::move(initializer.allowed) },
	  get_acceptance { std::move(initializer.get_acceptance) } {}

CancelableDiplomaticActionType::CancelableDiplomaticActionType(CancelableDiplomaticActionType::Initializer&& initializer)
	: allowed_to_cancel { std::move(initializer.allowed_cancel) }, DiplomaticActionType(std::move(initializer)) {}

DiplomaticActionManager::DiplomaticActionManager() {}

bool DiplomaticActionManager::add_diplomatic_action(
	std::string_view identifier, DiplomaticActionType::Initializer&& initializer
) {
	if (identifier.empty()) {
		Logger::error("Invalid diplomatic action identifier - empty!");
		return false;
	}
	return diplomatic_action_types.add_item({ identifier, DiplomaticActionType { std::move(initializer) } });
}

bool DiplomaticActionManager::add_cancelable_diplomatic_action(
	std::string_view identifier, CancelableDiplomaticActionType::Initializer&& initializer
) {
	if (identifier.empty()) {
		Logger::error("Invalid cancelable diplomatic action identifier - empty!");
		return false;
	}
	return diplomatic_action_types.add_item({ identifier, CancelableDiplomaticActionType { std::move(initializer) } });
}

DiplomaticActionTickCache DiplomaticActionManager::create_diplomatic_action_tick(
	std::string_view identifier, CountryInstance* sender, CountryInstance* reciever, std::any context_data
) {
	auto type = diplomatic_action_types.get_item_by_identifier(identifier);

	DiplomaticActionTickCache result { { sender, reciever, context_data }, type };
	type->visit([&](auto type) {
		if ((result.allowed_to_commit = type.allowed_to_commit(result.argument))) {
			result.acceptance = type.get_acceptance(result.argument);
		}
	});

	return result;
}

bool DiplomaticActionManager::setup_diplomatic_actions(GameManager& manager) {
	using Argument = DiplomaticActionType::Argument;

	bool result = true;

	result &= add_diplomatic_action(
		"form_alliance",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"call_ally",
		{
			.commit = [](Argument& arg) {},
			.allowed =
				[](const Argument& arg) {
					return false;
				},
			.get_acceptance =
				[](const Argument& arg) {
					return 1;
				},
		}
	);
	result &= add_cancelable_diplomatic_action(
		"request_military_access",
		{
			.commit = [](Argument& arg) {},
			.allowed_cancel =
				[](const Argument& arg) {
					return true;
				},
		}
	);
	result &= add_diplomatic_action("give_military_access", { [](Argument& arg) {} });
	result &= add_diplomatic_action(
		"increase_relations",
		{
			.commit =
				[&manager](Argument& arg) {
					auto relation = manager.get_diplomacy_manager().get_country_relation_manager().get_country_relation_ptr(
						arg.sender, arg.reciever
					);
					if (!relation) {
						return;
					}
					*relation += 25;
				},
			.allowed =
				[](const Argument& arg) {
					return false;
				},
		}
	);
	result &= add_diplomatic_action(
		"decrease_relations",
		{
			.commit =
				[&manager](Argument& arg) {
					auto relation = manager.get_diplomacy_manager().get_country_relation_manager().get_country_relation_ptr(
						arg.sender, arg.reciever
					);
					if (!relation) {
						return;
					}
					*relation -= 25;
				},
			.allowed =
				[](const Argument& arg) {
					return false;
				},
		}
	);
	result &= add_diplomatic_action(
		"war_subsidies",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"declare_war",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"offer_peace",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"command_units",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"discredit",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"expel_advisors",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"increase_opinion",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"decrease_opinion",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"add_to_sphere",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"remove_from_sphere",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"justify_war",
		{
			[](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"give_vision",
		{
			[](Argument& arg) {},
		}
	);
	diplomatic_action_types.lock();

	return result;
}
