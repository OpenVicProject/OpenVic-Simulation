#pragma once

#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"

namespace OpenVic {
	struct DecisionManager;

	struct Decision : HasIdentifier {
		friend struct DecisionManager;

	private:
		memory::string PROPERTY(news_title);
		memory::string PROPERTY(news_desc_long);
		memory::string PROPERTY(news_desc_medium);
		memory::string PROPERTY(news_desc_short);
		memory::string PROPERTY(picture);
		ConditionScript PROPERTY(potential);
		ConditionScript PROPERTY(allow);
		ConditionalWeightFactorMul PROPERTY(ai_will_do);
		EffectScript PROPERTY(effect);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		const bool has_alert;
		const bool is_news;

		Decision(
			std::string_view new_identifier, bool new_alert, bool new_news, std::string_view new_news_title,
			std::string_view new_news_desc_long, std::string_view new_news_desc_medium, std::string_view new_news_desc_short,
			std::string_view new_picture, ConditionScript&& new_potential, ConditionScript&& new_allow,
			ConditionalWeightFactorMul&& new_ai_will_do, EffectScript&& new_effect
		);
		Decision(Decision&&) = default;
	};
}
