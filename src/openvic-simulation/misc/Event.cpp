#include "Event.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/politics/Issue.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Event::EventOption::EventOption(std::string_view new_name, EffectScript&& new_effect, ConditionalWeight&& new_ai_chance)
  : name { new_name }, effect { std::move(new_effect) }, ai_chance { std::move(new_ai_chance) } {}

bool Event::EventOption::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	ret &= effect.parse_script(false, definition_manager);
	ret &= ai_chance.parse_scripts(definition_manager);
	return ret;
}

Event::Event(
	std::string_view new_identifier, std::string_view new_title, std::string_view new_description,
	std::string_view new_image, event_type_t new_type, bool new_triggered_only, bool new_major, bool new_fire_only_once,
	bool new_allows_multiple_instances, bool new_news, std::string_view new_news_title, std::string_view new_news_desc_long,
	std::string_view new_news_desc_medium, std::string_view new_news_desc_short, bool new_election,
	IssueGroup const* new_election_issue_group, ConditionScript&& new_trigger, ConditionalWeight&& new_mean_time_to_happen,
	EffectScript&& new_immediate, std::vector<EventOption>&& new_options
) : HasIdentifier { new_identifier }, title { new_title }, description { new_description }, image { new_image },
	type { new_type }, triggered_only { new_triggered_only }, major { new_major }, fire_only_once { new_fire_only_once },
	allows_multiple_instances { new_allows_multiple_instances }, news { new_news }, news_title { new_news_title },
	news_desc_long { new_news_desc_long }, news_desc_medium { new_news_desc_medium }, news_desc_short { new_news_desc_short },
	election { new_election }, election_issue_group { new_election_issue_group }, trigger { std::move(new_trigger) },
	mean_time_to_happen { std::move(new_mean_time_to_happen) }, immediate { std::move(new_immediate) },
	options { std::move(new_options) } {}

bool Event::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	ret &= trigger.parse_script(true, definition_manager);
	ret &= mean_time_to_happen.parse_scripts(definition_manager);
	ret &= immediate.parse_script(true, definition_manager);
	for (EventOption& option : options) {
		ret &= option.parse_scripts(definition_manager);
	}
	return ret;
}

OnAction::OnAction(std::string_view new_identifier, weight_map_t&& new_weighted_events)
	: HasIdentifier { new_identifier }, weighted_events { std::move(new_weighted_events) } {}

bool EventManager::register_event(
	std::string_view identifier, std::string_view title, std::string_view description, std::string_view image,
	Event::event_type_t type, bool triggered_only, bool major, bool fire_only_once, bool allows_multiple_instances, bool news,
	std::string_view news_title, std::string_view news_desc_long, std::string_view news_desc_medium,
	std::string_view news_desc_short, bool election, IssueGroup const* election_issue_group, ConditionScript&& trigger,
	ConditionalWeight&& mean_time_to_happen, EffectScript&& immediate, std::vector<Event::EventOption>&& options
) {
	if (identifier.empty()) {
		Logger::error("Invalid event ID - empty!");
		return false;
	}
	if (title.empty()) {
		Logger::warning("Event with ID ", identifier, " has no title!");
	}
	if (description.empty()) {
		Logger::warning("Event with ID ", identifier, " has no description!");
	}
	if (options.empty()) {
		Logger::error("No options specified for event with ID ", identifier);
		return false;
	}
	if (election && election_issue_group == nullptr) {
		Logger::warning("Event with ID ", identifier, " is an election event but has no issue group!");
	} else if (!election && election_issue_group != nullptr) {
		Logger::warning(
			"Event with ID ", identifier, " is not an election event but has issue group ",
			election_issue_group->get_identifier(), "!"
		);
	}
	if (news) {
		if (news_desc_long.empty() || news_desc_medium.empty() || news_desc_short.empty()) {
			Logger::warning(
				"Event with ID ", identifier, " is a news event but doesn't have long, medium and short descriptions!"
			);
		}
	} else {
		if (!news_title.empty() || !news_desc_long.empty() || !news_desc_medium.empty() || !news_desc_short.empty()) {
			Logger::warning("Event with ID ", identifier, " is not a news event but has news strings specified!");
		}
	}

	if (options.empty()) {
		Logger::error("Event with ID ", identifier, " has no options!");
		return false;
	} else {
		for (Event::EventOption const& option : options) {
			if (option.name.empty()) {
				Logger::warning("Event with ID ", identifier, " has an option with no name!");
			}
		}
	}

	// TODO - error if is_triggered_only with triggers or MTTH defined

	return events.add_item({
		identifier, title, description, image, type, triggered_only, major, fire_only_once, allows_multiple_instances, news,
		news_title, news_desc_long, news_desc_medium, news_desc_short, election, election_issue_group, std::move(trigger),
		std::move(mean_time_to_happen), std::move(immediate), std::move(options)
	}, duplicate_warning_callback);
}

bool EventManager::add_on_action(std::string_view identifier, OnAction::weight_map_t&& weighted_events) {
	if (identifier.empty()) {
		Logger::error("Invalid decision identifier - empty!");
		return false;
	}

	return on_actions.add_item({ identifier, std::move(weighted_events) });
}

bool EventManager::load_event_file(IssueManager const& issue_manager, ast::NodeCPtr root) {
	return expect_dictionary_reserve_length(
		events,
		[this, &issue_manager](std::string_view key, ast::NodeCPtr value) -> bool {
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
				Logger::error("Invalid event type: ", key);
				return false;
			}

			std::string_view identifier, title, description, image, news_title, news_desc_long, news_desc_medium,
				news_desc_short;
			bool triggered_only = false, major = false, fire_only_once = false, allows_multiple_instances = false,
				news = false, election = false;
			IssueGroup const* election_issue_group = nullptr;
			ConditionScript trigger { initial_scope, COUNTRY, NO_SCOPE };
			ConditionalWeight mean_time_to_happen { initial_scope, COUNTRY, NO_SCOPE };
			EffectScript immediate;
			std::vector<Event::EventOption> options;

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
				"issue_group", ZERO_OR_ONE,
					issue_manager.expect_issue_group_identifier(assign_variable_callback_pointer(election_issue_group)),
				"option", ONE_OR_MORE, [&options, initial_scope](ast::NodeCPtr node) -> bool {
					std::string_view name;
					EffectScript effect;
					ConditionalWeight ai_chance {
						initial_scope,
						COUNTRY,
						COUNTRY
					};

					bool ret = expect_dictionary_keys_and_default(
						key_value_success_callback, /* Option effects, passed to the EffectScript below */
						"name", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(name)),
						"ai_chance", ZERO_OR_ONE, ai_chance.expect_conditional_weight(ConditionalWeight::FACTOR)
					)(node);

					ret &= effect.expect_script()(node);

					options.push_back({ name, std::move(effect), std::move(ai_chance) });
					return ret;
				},
				"trigger", ZERO_OR_ONE, trigger.expect_script(),
				"mean_time_to_happen", ZERO_OR_ONE, mean_time_to_happen.expect_conditional_weight(ConditionalWeight::TIME),
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

bool EventManager::load_on_action_file(ast::NodeCPtr root) {
	bool ret = expect_dictionary([this](std::string_view identifier, ast::NodeCPtr node) -> bool {
		OnAction::weight_map_t weighted_events;
		bool ret = expect_dictionary_reserve_length(
			weighted_events,
			[this, &identifier, &weighted_events](std::string_view weight_str, ast::NodeCPtr event_node) -> bool {
				bool ret = false;
				uint64_t weight = StringUtils::string_to_uint64(weight_str, &ret);
				if (!ret) {
					Logger::error("Invalid weight ", weight_str, " on action ", identifier);
					return ret;
				}

				Event const* event = nullptr;
				ret &= expect_event_identifier(assign_variable_callback_pointer(event))(event_node);

				if (event != nullptr) {
					ret &= map_callback(weighted_events, event)(weight);
				} else {
					Logger::warning(
						"Non-existing event ", event_node, " loaded on action ", identifier, " with weight ",
						weight, "!"
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
