#include "Decision.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Decision::Decision(
	std::string_view new_identifier, bool new_alert, bool new_news, std::string_view new_news_title,
	std::string_view new_news_desc_long, std::string_view new_news_desc_medium, std::string_view new_news_desc_short,
	std::string_view new_picture, ConditionScript&& new_potential, ConditionScript&& new_allow,
	ConditionalWeight&& new_ai_will_do, EffectScript&& new_effect
) : HasIdentifier { new_identifier }, alert { new_alert }, news { new_news }, news_title { new_news_title },
	news_desc_long { new_news_desc_long }, news_desc_medium { new_news_desc_medium },
	news_desc_short { new_news_desc_short }, picture { new_picture }, potential { std::move(new_potential) },
	allow { std::move(new_allow) }, ai_will_do { std::move(new_ai_will_do) }, effect { std::move(new_effect) } {}

bool Decision::parse_scripts(GameManager& game_manager) {
	bool ret = true;
	ret &= potential.parse_script(false, game_manager);
	ret &= allow.parse_script(false, game_manager);
	ret &= ai_will_do.parse_scripts(game_manager);
	ret &= effect.parse_script(false, game_manager);
	return ret;
}

bool DecisionManager::add_decision(
	std::string_view identifier, bool alert, bool news, std::string_view news_title, std::string_view news_desc_long,
	std::string_view news_desc_medium, std::string_view news_desc_short, std::string_view picture, ConditionScript&& potential,
	ConditionScript&& allow, ConditionalWeight&& ai_will_do, EffectScript&& effect
) {
	if (identifier.empty()) {
		Logger::error("Invalid decision identifier - empty!");
		return false;
	}

	if (news) {
		if (news_desc_long.empty() || news_desc_medium.empty() || news_desc_short.empty()) {
			Logger::warning(
				"Decision with ID ", identifier, " is a news decision but doesn't have long, medium and short descriptions!"
			);
		}
	} else {
		if (!news_title.empty() || !news_desc_long.empty() || !news_desc_medium.empty() || !news_desc_short.empty()) {
			Logger::warning("Decision with ID ", identifier, " is not a news decision but has news strings specified!");
		}
	}

	return decisions.add_item({
		identifier, alert, news, news_title, news_desc_long, news_desc_medium, news_desc_short, picture, std::move(potential),
		std::move(allow), std::move(ai_will_do), std::move(effect)
	}, duplicate_warning_callback);
}

bool DecisionManager::load_decision_file(ast::NodeCPtr root) {
	return expect_dictionary_keys(
		"political_decisions", ZERO_OR_ONE, expect_dictionary(
			[this](std::string_view identifier, ast::NodeCPtr node) -> bool {
				bool alert = true, news = false;
				std::string_view news_title, news_desc_long, news_desc_medium, news_desc_short, picture;
				ConditionScript potential { scope_t::COUNTRY, scope_t::COUNTRY, scope_t::NO_SCOPE };
				ConditionScript allow { scope_t::COUNTRY, scope_t::COUNTRY, scope_t::NO_SCOPE };
				ConditionalWeight ai_will_do { scope_t::COUNTRY, scope_t::COUNTRY, scope_t::NO_SCOPE };
				EffectScript effect;
				bool ret = expect_dictionary_keys(
					"alert", ZERO_OR_ONE, expect_bool(assign_variable_callback(alert)),
					"news", ZERO_OR_ONE, expect_bool(assign_variable_callback(news)),
					"news_title", ZERO_OR_ONE, expect_string(assign_variable_callback(news_title)),
					"news_desc_long", ZERO_OR_ONE, expect_string(assign_variable_callback(news_desc_long)),
					"news_desc_medium", ZERO_OR_ONE, expect_string(assign_variable_callback(news_desc_medium)),
					"news_desc_short", ZERO_OR_ONE, expect_string(assign_variable_callback(news_desc_short)),
					"picture", ZERO_OR_ONE, expect_identifier_or_string(assign_variable_callback(picture)),
					"potential", ONE_EXACTLY, potential.expect_script(),
					"allow", ONE_EXACTLY, allow.expect_script(),
					"effect", ONE_EXACTLY, effect.expect_script(),
					"ai_will_do", ZERO_OR_ONE, ai_will_do.expect_conditional_weight(ConditionalWeight::FACTOR)
				)(node);
				ret &= add_decision(
					identifier, alert, news, news_title, news_desc_long, news_desc_medium, news_desc_short, picture,
					std::move(potential), std::move(allow), std::move(ai_will_do), std::move(effect)
				);
				return ret;
			}
		)
	)(root);
}

bool DecisionManager::parse_scripts(GameManager& game_manager) {
	bool ret = true;
	for (Decision& decision : decisions.get_items()) {
		ret &= decision.parse_scripts(game_manager);
	}
	return ret;
}
