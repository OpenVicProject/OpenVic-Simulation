#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct DecisionManager;

	struct Decision : HasIdentifier {
		friend struct DecisionManager;

	private:
		const bool PROPERTY_CUSTOM_PREFIX(alert, has);
		const bool PROPERTY_CUSTOM_PREFIX(news, is);
		const std::string PROPERTY(news_title);
		const std::string PROPERTY(news_desc_long);
		const std::string PROPERTY(news_desc_medium);
		const std::string PROPERTY(news_desc_short);
		const std::string PROPERTY(picture);

		Decision(
			std::string_view new_identifier, bool new_alert, bool new_news,
			std::string_view new_news_title, std::string_view new_news_desc_long,
			std::string_view new_news_desc_medium, std::string_view new_news_desc_short,
			std::string_view new_picture
		);

	public:
		Decision(Decision&&) = default;
	};

	struct DecisionManager {
	private:
		IdentifierRegistry<Decision> IDENTIFIER_REGISTRY(decision);

	public:
		bool add_decision(
			std::string_view identifier, bool alert, bool news, std::string_view news_title,
			std::string_view news_desc_long, std::string_view news_desc_medium,
			std::string_view news_desc_short, std::string_view picture
		);

		bool load_decision_file(ast::NodeCPtr root);
	};
}
