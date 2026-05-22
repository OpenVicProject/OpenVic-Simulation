#pragma once

#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/politics/IssueGroup.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct EventManager;

	struct Event : HasIdentifier {
		friend struct EventManager;

		enum struct event_type_t : uint8_t { COUNTRY, PROVINCE };

		struct EventOption {
			friend struct EventManager;
			friend struct Event;

		private:
			memory::string PROPERTY(name);
			EffectScript PROPERTY(effect);
			ConditionalWeightFactorMul PROPERTY(ai_chance);

			bool parse_scripts(DefinitionManager const& definition_manager);

		public:
			EventOption(std::string_view new_name, EffectScript&& new_effect, ConditionalWeightFactorMul&& new_ai_chance);
			EventOption(EventOption const&) = delete;
			EventOption(EventOption&&) = default;
			EventOption& operator=(EventOption const&) = delete;
			EventOption& operator=(EventOption&&) = delete;
		};

	private:
		memory::string PROPERTY(title);
		memory::string PROPERTY(description);
		memory::string PROPERTY(image);
		event_type_t PROPERTY(type);
		memory::string PROPERTY(news_title);
		memory::string PROPERTY(news_desc_long);
		memory::string PROPERTY(news_desc_medium);
		memory::string PROPERTY(news_desc_short);

		bool PROPERTY_CUSTOM_PREFIX(election, is);
		issue_group_t PROPERTY(election_issue_group);

		ConditionScript PROPERTY(trigger);
		ConditionalWeightTime PROPERTY(mean_time_to_happen);
		EffectScript PROPERTY(immediate);

		memory::vector<EventOption> SPAN_PROPERTY(options);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		const bool is_triggered_only;
		const bool is_major;
		const bool fire_only_once;
		const bool allows_multiple_instances;
		const bool is_news;

		Event(
			std::string_view new_identifier, std::string_view new_title, std::string_view new_description,
			std::string_view new_image, event_type_t new_type, bool new_triggered_only, bool new_major,
			bool new_fire_only_once, bool new_allows_multiple_instances, bool new_news, std::string_view new_news_title,
			std::string_view new_news_desc_long, std::string_view new_news_desc_medium, std::string_view new_news_desc_short,
			bool new_election, const issue_group_t new_election_issue_group, ConditionScript&& new_trigger,
			ConditionalWeightTime&& new_mean_time_to_happen, EffectScript&& new_immediate,
			memory::vector<EventOption>&& new_options
		);
		Event(Event&&) = default;
	};

	struct OnAction : HasIdentifier {
		using weight_map_t = ordered_map<Event const*, uint64_t>;

	private:
		weight_map_t PROPERTY(weighted_events);

	public:
		OnAction(std::string_view new_identifier, weight_map_t&& new_weighted_events);
		OnAction(OnAction&&) = default;
	};
}
