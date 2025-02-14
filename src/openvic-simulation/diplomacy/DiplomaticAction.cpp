#include "DiplomaticAction.hpp"

#include <any>
#include <string_view>

#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

DiplomaticActionType::DiplomaticActionType(DiplomaticActionType::Initializer&& initializer)
	: commit_action_caller { std::move(initializer.commit) }, allowed_to_commit { std::move(initializer.allowed) },
	  get_acceptance { std::move(initializer.get_acceptance) } {}

CancelableDiplomaticActionType::CancelableDiplomaticActionType(CancelableDiplomaticActionType::Initializer&& initializer)
	: allowed_to_cancel { std::move(initializer.allowed_cancel) }, DiplomaticActionType { std::move(initializer) } {}

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
	std::string_view identifier, CountryInstance* sender, CountryInstance* reciever, std::any context_data,
	InstanceManager& instance_manager
) {
	auto type = diplomatic_action_types.get_item_by_identifier(identifier);

	DiplomaticActionTickCache result { { instance_manager, sender, reciever, context_data }, type };
	type->visit([&](auto type) {
		CountryRelationManager::influence_value_type* influence = nullptr;
		if (type.influence_cost != 0) {
			influence = &result.argument.instance_manager.get_country_relation_manager().assign_or_get_country_influence(
				result.argument.sender, result.argument.reciever
			);
			if (*influence < type.influence_cost) {
				return;
			}
		}
		if (!(result.allowed_to_commit = type.allowed_to_commit(result.argument))) {
			return;
		}
		result.acceptance = type.get_acceptance(result.argument);
		if (influence != nullptr) {
			*influence -= type.influence_cost;
		}
	});

	return result;
}

bool DiplomaticActionManager::setup_diplomatic_actions() {
	using Argument = DiplomaticActionType::Argument;

	bool result = true;

	result &= add_diplomatic_action(
		"form_alliance",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_alliance(
						arg.sender, arg.reciever, true
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"end_alliance",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_alliance(
						arg.sender, arg.reciever, false
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"call_ally",
		{
			.commit = [](Argument& arg) {},
			.allowed =
				[](Argument const& arg) {
					return false;
				},
			.get_acceptance =
				[](Argument const& arg) {
					return 1;
				},
		}
	);
	result &= add_diplomatic_action(
		"request_military_access",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_military_access(
						arg.sender, arg.reciever, true
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"give_military_access",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_military_access(
						arg.sender, arg.reciever, true
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"increase_relations",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().assign_or_get_country_relation(
						arg.sender, arg.reciever
					) += 25;
				},
			.allowed =
				[](Argument const& arg) {
					return false;
				},
		}
	);
	result &= add_diplomatic_action(
		"decrease_relations",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().assign_or_get_country_relation(
						arg.sender, arg.reciever
					) -= 25;
				},
			.allowed =
				[](Argument const& arg) {
					return false;
				},
		}
	);
	result &= add_diplomatic_action(
		"war_subsidies",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_war_subsidies(
						arg.sender, arg.reciever, true
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"end_war_subsidies",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_war_subsidies(
						arg.sender, arg.reciever, false
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"declare_war",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_at_war(
						arg.sender, arg.reciever, true
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"offer_peace",
		{
			.commit = [](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"command_units",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().assign_or_get_country_command_units(
						arg.sender, arg.reciever
					) = true;
				},
		}
	);
	result &= add_diplomatic_action(
		"discredit",
		{
			.influence_cost = 25,
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_discredit(
						arg.sender, arg.reciever, true
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"expel_advisors",
		{
			.influence_cost = 50,
			.commit = [](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"ban_embassy",
		{
			.influence_cost = 65,
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_embassy_ban(
						arg.sender, arg.reciever, true
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"increase_opinion",
		{
			.influence_cost = 50,
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().assign_or_get_country_opinion(
						arg.sender, arg.reciever
					)++;
				},
			.allowed =
				[](Argument const& arg) {
					return true;
				},
		}
	);
	result &= add_diplomatic_action(
		"decrease_opinion",
		{
			.influence_cost = 50,
			.commit =
				[](Argument& arg) {
					--arg.instance_manager.get_country_relation_manager().assign_or_get_country_opinion(
						std::any_cast<CountryInstance*>(arg.context_data), arg.reciever
					);
				},
			.allowed =
				[](Argument const& arg) {
					return true;
				},
		}
	);
	result &= add_diplomatic_action(
		"add_to_sphere",
		{
			.influence_cost = 100,
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().assign_or_get_country_opinion(
						arg.sender, arg.reciever
					) = CountryRelationManager::OpinionType::Sphere;
				},
			.allowed =
				[](Argument const& arg) {
					return true;
				},
		}
	);
	result &= add_diplomatic_action(
		"remove_from_foreign_sphere",
		{
			.influence_cost = 100,
			.commit =
				[](Argument& arg) {
					--arg.instance_manager.get_country_relation_manager().assign_or_get_country_opinion(
						std::any_cast<CountryInstance*>(arg.context_data), arg.reciever
					);
					arg.instance_manager.get_country_relation_manager().assign_or_get_country_relation(
						arg.sender, arg.reciever
					) -= 10;
				},
			.allowed =
				[](Argument const& arg) {
					return true;
				},
		}
	);
	result &= add_diplomatic_action(
		"remove_from_domestic_sphere",
		{
			.influence_cost = 100,
			.commit =
				[](Argument& arg) {
					--arg.instance_manager.get_country_relation_manager().assign_or_get_country_opinion(
						arg.sender, arg.reciever
					);
					arg.instance_manager.get_country_relation_manager().assign_or_get_country_relation(
						arg.sender, arg.reciever
					) -= 10;
					// TODO: subtract 10 prestige from arg.sender
				},
			.allowed =
				[](Argument const& arg) {
					return true;
				},
		}
	);
	result &= add_diplomatic_action(
		"justify_war",
		{
			.commit = [](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"give_vision",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_vision(
						arg.sender, arg.reciever, true
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"remove_vision",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_vision(
						arg.sender, arg.reciever, false
					);
				},
		}
	);
	diplomatic_action_types.lock();

	return result;
}
