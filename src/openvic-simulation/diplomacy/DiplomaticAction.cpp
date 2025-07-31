#include "DiplomaticAction.hpp"

#include <any>
#include <string_view>

#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/types/Date.hpp"
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
	return diplomatic_action_types.emplace_item( identifier, identifier, DiplomaticActionType { std::move(initializer) } );
}

bool DiplomaticActionManager::add_cancelable_diplomatic_action(
	std::string_view identifier, CancelableDiplomaticActionType::Initializer&& initializer
) {
	if (identifier.empty()) {
		Logger::error("Invalid cancelable diplomatic action identifier - empty!");
		return false;
	}
	return diplomatic_action_types.emplace_item( identifier, identifier, CancelableDiplomaticActionType { std::move(initializer) } );
}

DiplomaticActionTickCache DiplomaticActionManager::create_diplomatic_action_tick(
	std::string_view identifier, CountryInstance* sender, CountryInstance* receiver, std::any context_data,
	InstanceManager& instance_manager
) {
	auto type = diplomatic_action_types.get_item_by_identifier(identifier);

	DiplomaticActionTickCache result { { instance_manager, sender, receiver, context_data }, type };
	type->visit([&](auto type) {
		CountryRelationManager::influence_value_type* influence = nullptr;
		if (type.influence_cost != 0) {
			influence = &result.argument.instance_manager.get_country_relation_manager().assign_or_get_influence_with( //
				result.argument.sender, result.argument.receiver
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

	// TODO: implement  _RELATION_ON_ACCEPT & _RELATION_ON_DECLINE

	result &= add_diplomatic_action(
		"form_alliance",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_alliance( //
						arg.sender, arg.receiver, true
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"end_alliance",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_country_alliance( //
						arg.sender, arg.receiver, false
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
					arg.instance_manager.get_country_relation_manager().set_has_military_access_to( //
						arg.sender, arg.receiver, true
					);
				},
		}
	);
	result &= add_diplomatic_action( //
		"give_military_access",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_has_military_access_to( //
						arg.sender, arg.receiver, true
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
						arg.sender, arg.receiver
					) += 25; // TODO: implement defines.diplomacy.INCREASERELATION_RELATION_ON_ACCEPT
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
						arg.sender, arg.receiver
					) -= 25; // TODO: implement defines.diplomacy.DECREASERELATION_RELATION_ON_ACCEPT
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
					arg.instance_manager.get_country_relation_manager().set_war_subsidies_to( //
						arg.sender, arg.receiver, true
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"end_war_subsidies",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_war_subsidies_to( //
						arg.sender, arg.receiver, false
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"declare_war",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_at_war_with( //
						arg.sender, arg.receiver, true
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
					arg.instance_manager.get_country_relation_manager().assign_or_get_commands_units( //
						arg.sender, arg.receiver
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
					arg.instance_manager.get_country_relation_manager().set_discredited_date( //
						arg.sender, arg.receiver, arg.instance_manager.get_today() + 180
					); // TODO: implement defines.diplomacy.DISCREDIT_DAYS
				},
		}
	);
	result &= add_diplomatic_action(
		"expel_advisors",
		{
			.influence_cost = 50, // TODO: implement defines.diplomacy.EXPELADVISORS_INFLUENCE_COST
			.commit = [](Argument& arg) {},
		}
	);
	result &= add_diplomatic_action(
		"ban_embassy",
		{
			.influence_cost = 65,
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_embassy_banned_date( //
						arg.sender, arg.receiver, arg.instance_manager.get_today() + Timespan::from_years(1)
					); // TODO: implement defines.diplomacy.BANEMBASSY_DAYS
				},
		}
	);
	result &= add_diplomatic_action(
		"increase_opinion",
		{
			.influence_cost = 50,
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().assign_or_get_country_opinion( //
						arg.sender, arg.receiver
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
					--arg.instance_manager.get_country_relation_manager().assign_or_get_country_opinion( //
						std::any_cast<CountryInstance*>(arg.context_data), arg.receiver
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
					arg.instance_manager.get_country_relation_manager().assign_or_get_country_opinion( //
						arg.sender, arg.receiver
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
					--arg.instance_manager.get_country_relation_manager().assign_or_get_country_opinion( //
						std::any_cast<CountryInstance*>(arg.context_data), arg.receiver
					);
					arg.instance_manager.get_country_relation_manager().assign_or_get_country_relation( //
						arg.sender, arg.receiver
					) -= 10;
				},
			.allowed =
				[](Argument const& arg) {
					return true;
				},
		}
	);
	result &= add_diplomatic_action(
		"remove_from_own_sphere",
		{
			.influence_cost = 100,
			.commit =
				[](Argument& arg) {
					--arg.instance_manager.get_country_relation_manager().assign_or_get_country_opinion( //
						arg.sender, arg.receiver
					);
					arg.instance_manager.get_country_relation_manager().assign_or_get_country_relation( //
						arg.sender, arg.receiver
					) -= 10;
					// TODO: implement REMOVEFROMSPHERE_PRESTIGE_COST and REMOVEFROMSPHERE_INFAMY_COST
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
					arg.instance_manager.get_country_relation_manager().set_has_vision( //
						arg.sender, arg.receiver, true
					);
				},
		}
	);
	result &= add_diplomatic_action(
		"remove_vision",
		{
			.commit =
				[](Argument& arg) {
					arg.instance_manager.get_country_relation_manager().set_has_vision( //
						arg.sender, arg.receiver, false
					);
				},
		}
	);
	diplomatic_action_types.lock();

	return result;
}
