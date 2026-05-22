#include "Event.hpp"

using namespace OpenVic;

Event::EventOption::EventOption(
	std::string_view new_name, EffectScript&& new_effect, ConditionalWeightFactorMul&& new_ai_chance
) : name { new_name }, effect { std::move(new_effect) }, ai_chance { std::move(new_ai_chance) } {}

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
	const issue_group_t new_election_issue_group, ConditionScript&& new_trigger, ConditionalWeightTime&& new_mean_time_to_happen,
	EffectScript&& new_immediate, memory::vector<EventOption>&& new_options
) : HasIdentifier { new_identifier }, title { new_title }, description { new_description }, image { new_image },
	type { new_type }, is_triggered_only { new_triggered_only }, is_major { new_major }, fire_only_once { new_fire_only_once },
	allows_multiple_instances { new_allows_multiple_instances }, is_news { new_news }, news_title { new_news_title },
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
