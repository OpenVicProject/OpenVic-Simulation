#include "Event.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/politics/Issue.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Event::EventOption::EventOption(std::string_view new_title) : title { new_title } {}

Event::Event(
	std::string_view new_identifier, std::string_view new_title, std::string_view new_description,
	std::string_view new_image, event_type_t new_type, bool new_triggered_only, bool new_major, bool new_fire_only_once,
	bool new_allows_multiple_instances, bool new_news, std::string_view new_news_title, std::string_view new_news_desc_long,
	std::string_view new_news_desc_medium, std::string_view new_news_desc_short, bool new_election,
	IssueGroup const* new_election_issue_group, std::vector<EventOption>&& new_options
) : HasIdentifier { new_identifier }, title { new_title }, description { new_description }, image { new_image },
	type { new_type }, triggered_only { new_triggered_only }, major { new_major }, fire_only_once { new_fire_only_once },
	allows_multiple_instances { new_allows_multiple_instances }, news { new_news }, news_title { new_news_title },
	news_desc_long { new_news_desc_long }, news_desc_medium { new_news_desc_medium }, news_desc_short { new_news_desc_short },
	election { new_election }, election_issue_group { new_election_issue_group }, options { std::move(new_options) } {}

bool EventManager::register_event(
	std::string_view identifier, std::string_view title, std::string_view description, std::string_view image,
	Event::event_type_t type, bool triggered_only, bool major, bool fire_only_once, bool allows_multiple_instances, bool news,
	std::string_view news_title, std::string_view news_desc_long, std::string_view news_desc_medium,
	std::string_view news_desc_short, bool election, IssueGroup const* election_issue_group,
	std::vector<Event::EventOption>&& options
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

	// TODO - error if is_triggered_only with triggers or MTTH defined

	return events.add_item({
		identifier, title, description, image, type, triggered_only, major, fire_only_once, allows_multiple_instances, news,
		news_title, news_desc_long, news_desc_medium, news_desc_short, election, election_issue_group, std::move(options)
	}, duplicate_warning_callback);
}

bool EventManager::load_event_file(IssueManager const& issue_manager, ast::NodeCPtr root) {
	return expect_dictionary(
		[this, &issue_manager](std::string_view key, ast::NodeCPtr value) -> bool {
			Event::event_type_t type;
			std::string_view identifier, title, description, image, news_title, news_desc_long, news_desc_medium,
				news_desc_short;
			bool triggered_only = false, major = false, fire_only_once = false, allows_multiple_instances = false,
				news = false, election = false;
			IssueGroup const* election_issue_group = nullptr;
			std::vector<Event::EventOption> options;

			if (key == "country_event") {
				type = Event::event_type_t::COUNTRY;
			} else if (key == "province_event") {
				type = Event::event_type_t::PROVINCE;
			} else {
				Logger::error("Invalid event type: ", key);
				return false;
			}

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
				"option", ONE_OR_MORE, [&options](ast::NodeCPtr node) -> bool {
					std::string_view title;

					bool ret = expect_dictionary_keys_and_default(
						key_value_success_callback,
						"name", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(title))
					)(node);

					// TODO: option effects

					options.push_back({ title });
					return ret;
				},
				"trigger", ZERO_OR_ONE, success_callback, // TODO - trigger condition
				"mean_time_to_happen", ZERO_OR_ONE, success_callback, // TODO - MTTH weighted conditions
				"immediate", ZERO_OR_MORE, success_callback // TODO - immediate effects
			)(value);
			ret &= register_event(
				identifier, title, description, image, type, triggered_only, major, fire_only_once, allows_multiple_instances,
				news, news_title, news_desc_long, news_desc_medium, news_desc_short, election, election_issue_group,
				std::move(options)
			);
			return ret;
		}
	)(root);
}
