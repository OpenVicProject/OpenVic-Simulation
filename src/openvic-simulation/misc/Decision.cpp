#include "Decision.hpp"


using namespace OpenVic;

Decision::Decision(
	std::string_view new_identifier, bool new_alert, bool new_news, std::string_view new_news_title,
	std::string_view new_news_desc_long, std::string_view new_news_desc_medium, std::string_view new_news_desc_short,
	std::string_view new_picture, ConditionScript&& new_potential, ConditionScript&& new_allow,
	ConditionalWeightFactorMul&& new_ai_will_do, EffectScript&& new_effect
) : HasIdentifier { new_identifier }, has_alert { new_alert }, is_news { new_news }, news_title { new_news_title },
	news_desc_long { new_news_desc_long }, news_desc_medium { new_news_desc_medium },
	news_desc_short { new_news_desc_short }, picture { new_picture }, potential { std::move(new_potential) },
	allow { std::move(new_allow) }, ai_will_do { std::move(new_ai_will_do) }, effect { std::move(new_effect) } {}

bool Decision::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	ret &= potential.parse_script(false, definition_manager);
	ret &= allow.parse_script(false, definition_manager);
	ret &= ai_will_do.parse_scripts(definition_manager);
	ret &= effect.parse_script(false, definition_manager);
	return ret;
}
