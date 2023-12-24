#include "Decision.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Decision::Decision(
	std::string_view new_identifier, bool new_alert, bool new_news, std::string_view new_news_title,
	std::string_view new_news_desc_long, std::string_view new_news_desc_medium, std::string_view new_news_desc_short,
	std::string_view new_picture
) : HasIdentifier { new_identifier }, alert { new_alert }, news { new_news }, news_title { new_news_title },
	news_desc_long { new_news_desc_long }, news_desc_medium { new_news_desc_medium },
	news_desc_short { new_news_desc_short }, picture { new_picture } {}

bool DecisionManager::add_decision(
	std::string_view identifier, bool alert, bool news, std::string_view news_title, std::string_view news_desc_long,
	std::string_view news_desc_medium, std::string_view news_desc_short, std::string_view picture
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
		identifier, alert, news, news_title, news_desc_long, news_desc_medium, news_desc_short, picture
	}, duplicate_warning_callback);
}

bool DecisionManager::load_decision_file(ast::NodeCPtr root) {
	return expect_dictionary_keys(
		"political_decisions", ZERO_OR_ONE, expect_dictionary(
			[this](std::string_view identifier, ast::NodeCPtr node) -> bool {
				bool alert = true, news = false;
				std::string_view news_title, news_desc_long, news_desc_medium, news_desc_short, picture;
				bool ret = expect_dictionary_keys(
					"alert", ZERO_OR_ONE, expect_bool(assign_variable_callback(alert)),
					"news", ZERO_OR_ONE, expect_bool(assign_variable_callback(news)),
					"news_title", ZERO_OR_ONE, expect_string(assign_variable_callback(news_title)),
					"news_desc_long", ZERO_OR_ONE, expect_string(assign_variable_callback(news_desc_long)),
					"news_desc_medium", ZERO_OR_ONE, expect_string(assign_variable_callback(news_desc_medium)),
					"news_desc_short", ZERO_OR_ONE, expect_string(assign_variable_callback(news_desc_short)),
					"picture", ZERO_OR_ONE, expect_identifier_or_string(assign_variable_callback(picture)),
					"potential", ONE_EXACTLY, success_callback, //TODO
					"allow", ONE_EXACTLY, success_callback, //TODO
					"effect", ONE_EXACTLY, success_callback, //TODO
					"ai_will_do", ZERO_OR_ONE, success_callback //TODO
				)(node);
				ret &= add_decision(
					identifier, alert, news, news_title, news_desc_long, news_desc_medium, news_desc_short, picture
				);
				return ret;
			}
		)
	)(root);
}
