#pragma once

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/misc/Event.hpp"
#include "openvic-simulation/politics/IssueGroup.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct IssueManager;

	struct EventManager {
	private:
		IdentifierRegistry<Event> IDENTIFIER_REGISTRY(event);
		IdentifierRegistry<OnAction> IDENTIFIER_REGISTRY(on_action);

	public:
		EventManager();

		bool register_event(
			std::string_view identifier, std::string_view title, std::string_view description, std::string_view image,
			Event::event_type_t type, bool triggered_only, bool major, bool fire_only_once, bool allows_multiple_instances,
			bool news, std::string_view news_title, std::string_view news_desc_long, std::string_view news_desc_medium,
			std::string_view news_desc_short, bool election, const issue_group_t election_issue_group, ConditionScript&& trigger,
			ConditionalWeightTime&& mean_time_to_happen, EffectScript&& immediate, memory::vector<Event::EventOption>&& options
		);

		bool add_on_action(std::string_view identifier, OnAction::weight_map_t&& new_weighted_events);

		bool load_event_file(IssueManager const& issue_manager, ovdl::v2script::ast::Node const* root);
		bool load_on_action_file(ovdl::v2script::ast::Node const* root);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
