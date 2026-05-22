#include "EventManager.hpp"

#include <charconv>
#include <system_error>

#include <fmt/std.h>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/politics/IssueManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

EventManager::EventManager() {
	// Default total vanilla events is 1064
	events.reserve(1064);
}

bool EventManager::register_event(
	std::string_view identifier, std::string_view title, std::string_view description, std::string_view image,
	Event::event_type_t type, bool triggered_only, bool major, bool fire_only_once, bool allows_multiple_instances, bool news,
	std::string_view news_title, std::string_view news_desc_long, std::string_view news_desc_medium,
	std::string_view news_desc_short, bool election, const issue_group_t election_issue_group, ConditionScript&& trigger,
	ConditionalWeightTime&& mean_time_to_happen, EffectScript&& immediate, memory::vector<Event::EventOption>&& options
) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid event ID - empty!");
		return false;
	}
	if (title.empty()) {
		spdlog::warn_s("Event with ID {} has no title!", identifier);
	}
	if (description.empty()) {
		spdlog::warn_s("Event with ID {} has no description!", identifier);
	}
	if (options.empty()) {
		spdlog::error_s("No options specified for event with ID {}", identifier);
		return false;
	}
	if (election && std::holds_alternative<std::monostate>(election_issue_group)) {
		spdlog::warn_s("Event with ID {} is an election event but has no issue group!", identifier);
	} else if (!election) {
		std::visit([identifier](auto&& arg) {
			using T = std::decay_t<decltype(arg)>; // Get the clean type
			
			if constexpr (std::is_same_v<T, std::reference_wrapper<const PartyPolicyGroup>>) {
				spdlog::warn_s(
					"Event with ID {} is not an election event but has party policy group {}!",
					identifier, arg
				);
			} else if constexpr (std::is_same_v<T, std::reference_wrapper<const ReformGroup>>) {
				spdlog::warn_s(
					"Event with ID {} is not an election event but has reform group {}!",
					identifier, arg
				);
			} else if constexpr (!std::is_same_v<T, std::monostate>){
				spdlog::error_s(
					"Event with ID {} is not an election event but has unknown issue group {}!",
					identifier, arg
				);
			}
		}, election_issue_group);
	}
	if (news) {
		if (news_desc_long.empty() || news_desc_medium.empty() || news_desc_short.empty()) {
			spdlog::warn_s(
				"Event with ID {} is a news event but doesn't have long, medium and short descriptions!", identifier
			);
		}
	} else {
		if (!news_title.empty() || !news_desc_long.empty() || !news_desc_medium.empty() || !news_desc_short.empty()) {
			spdlog::warn_s("Event with ID {} is not a news event but has news strings specified!", identifier);
		}
	}

	for (Event::EventOption const& option : options) {
		if (option.name.empty()) {
			spdlog::warn_s("Event with ID {} has an option with no name!", identifier);
		}
	}

	// TODO - error if is_triggered_only with triggers or MTTH defined

	return events.emplace_item(
		identifier,
		duplicate_warning_callback,
		identifier, title, description, image, type, triggered_only, major, fire_only_once, allows_multiple_instances, news,
		news_title, news_desc_long, news_desc_medium, news_desc_short, election, election_issue_group, std::move(trigger),
		std::move(mean_time_to_happen), std::move(immediate), std::move(options)
	);
}

bool EventManager::add_on_action(std::string_view identifier, OnAction::weight_map_t&& weighted_events) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid on_action identifier - empty!");
		return false;
	}

	return on_actions.emplace_item(
		identifier,
		identifier, std::move(weighted_events)
	);
}

bool EventManager::load_event_file(IssueManager const& issue_manager, ovdl::v2script::ast::Node const* root) {
	return expect_dictionary_reserve_length(
		events,
		[this, &issue_manager](std::string_view key, ovdl::v2script::ast::Node const* value) -> bool {
			using enum scope_type_t;

			Event::event_type_t type;
			scope_type_t initial_scope;

			if (key == "country_event") {
				type = Event::event_type_t::COUNTRY;
				initial_scope = COUNTRY;
			} else if (key == "province_event") {
				type = Event::event_type_t::PROVINCE;
				initial_scope = PROVINCE;
			} else {
				spdlog::error_s("Invalid event type: {}", key);
				return false;
			}

			std::string_view identifier, title, description, image, news_title, news_desc_long, news_desc_medium,
				news_desc_short;
			bool triggered_only = false, major = false, fire_only_once = false, allows_multiple_instances = false,
				news = false, election = false;
			issue_group_t election_issue_group;
			ConditionScript trigger { initial_scope, COUNTRY, NO_SCOPE };
			ConditionalWeightTime mean_time_to_happen { initial_scope, COUNTRY, NO_SCOPE };
			EffectScript immediate;

			// Ensures that if ever multithreaded, only one vector is used per thread
			// Else acts like static
			thread_local memory::vector<Event::EventOption> options;
			// Default max vanilla options is 5
			options.reserve(2);

			bool ret = expect_dictionary_keys(
				"id", ONE_EXACTLY, expect_identifier(assign_variable_callback(identifier)),
				"title", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(title)),
				"desc", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(description)),
				"picture", ZERO_OR_ONE, expect_identifier_or_string(assign_variable_callback(image), true),
				"is_triggered_only", ZERO_OR_ONE, expect_bool(assign_variable_callback(triggered_only)),
				"major", ZERO_OR_ONE, expect_bool(assign_variable_callback(major)),
				"fire_only_once", ZERO_OR_ONE, expect_bool(assign_variable_callback(fire_only_once)),
				"allow_multiple_instances", ZERO_OR_ONE, expect_bool(assign_variable_callback(allows_multiple_instances)),
				"news", ZERO_OR_ONE, expect_bool(assign_variable_callback(news)),
				"news_title", ZERO_OR_ONE, expect_identifier_or_string(assign_variable_callback(news_title)),
				"news_desc_long", ZERO_OR_ONE, expect_identifier_or_string(assign_variable_callback(news_desc_long)),
				"news_desc_medium", ZERO_OR_ONE, expect_identifier_or_string(assign_variable_callback(news_desc_medium)),
				"news_desc_short", ZERO_OR_ONE, expect_identifier_or_string(assign_variable_callback(news_desc_short)),
				"election", ZERO_OR_ONE, expect_bool(assign_variable_callback(election)),
				"issue_group", ZERO_OR_ONE, issue_manager.expect_base_issue_group_identifier(assign_variable_callback(election_issue_group)),
				"option", ONE_OR_MORE, [initial_scope](ovdl::v2script::ast::Node const* node) -> bool {
					std::string_view name;
					EffectScript effect;
					ConditionalWeightFactorMul ai_chance {
						initial_scope,
						COUNTRY,
						COUNTRY
					};

					bool ret = expect_dictionary_keys_and_default(
						key_value_success_callback, /* Option effects, passed to the EffectScript below */
						"name", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(name)),
						"ai_chance", ZERO_OR_ONE, ai_chance.expect_conditional_weight()
					)(node);

					ret &= effect.expect_script()(node);

					options.emplace_back(name, std::move(effect), std::move(ai_chance));
					return ret;
				},
				"trigger", ZERO_OR_ONE, trigger.expect_script(),
				"mean_time_to_happen", ZERO_OR_ONE, mean_time_to_happen.expect_conditional_weight(),
				"immediate", ZERO_OR_MORE, immediate.expect_script()
			)(value);

			ret &= register_event(
				identifier, title, description, image, type, triggered_only, major, fire_only_once, allows_multiple_instances,
				news, news_title, news_desc_long, news_desc_medium, news_desc_short, election, election_issue_group,
				std::move(trigger), std::move(mean_time_to_happen), std::move(immediate), std::move(options)
			);
			return ret;
		}
	)(root);
}

bool EventManager::load_on_action_file(ovdl::v2script::ast::Node const* root) {
	bool ret = expect_dictionary([this](std::string_view identifier, ovdl::v2script::ast::Node const* node) -> bool {
		OnAction::weight_map_t weighted_events;
		bool ret = expect_dictionary_reserve_length(
			weighted_events,
			[this, &identifier, &weighted_events](std::string_view weight_str, ovdl::v2script::ast::Node const* event_node) -> bool {
				bool ret = false;
				uint64_t weight;
				std::from_chars_result result = string_to_uint64(weight_str, weight);
				ret = result.ec == std::errc{};
				if (!ret) {
					spdlog::error_s("Invalid weight {} on action {}", weight_str, identifier);
					return ret;
				}

				Event const* event = nullptr;
				ret &= expect_event_identifier(assign_variable_callback_pointer(event))(event_node);

				if (event != nullptr) {
					ret &= map_callback(weighted_events, event)(weight);
				} else if (
					ovdl::v2script::ast::FlatValue const* event_name = dryad::node_try_cast<ovdl::v2script::ast::FlatValue>(event_node);
					event_name && event_name->value()
				) {
					spdlog::warn_s(
						"Non-existing event {} loaded on action {} with weight {}!",
						event_name->value().view(), identifier, weight
					);
				}

				return ret;
			}
		)(node);
		ret &= add_on_action(identifier, std::move(weighted_events));
		return ret;
	})(root);
	on_actions.lock();
	return ret;
}

bool EventManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	for (Event& event : events.get_items()) {
		ret &= event.parse_scripts(definition_manager);
	}
	return ret;
}
