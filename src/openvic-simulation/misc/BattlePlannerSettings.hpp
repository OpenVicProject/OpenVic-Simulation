#pragma once

#include <string>
#include <string_view>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"


namespace OpenVic {
	struct BattlePlannerSettingsManager;

	struct Tool : HasIdentifier {
		friend struct BattlePlannerSettingsManager;

	private:
		struct Option {
			friend struct Tool;

			std::string name;
			std::optional<std::string> type;
			std::optional<colour_t> colour;

			Option(std::string_view new_name);
		public:
			Option(Option&&) = default;

		};

		std::string PROPERTY(icon_identifier);
		std::vector<Option> PROPERTY(options);

	//private:
	//	bool _set_options(std::vector<Option> options);
	public:
		Tool(std::string_view new_identifier, std::string_view new_icon_identifier, std::vector<Option> new_options);


	public:
		Tool(Tool&&) = default;
	};

	struct BattlePlannerSettingsManager {
	private:
		IdentifierRegistry<Tool> IDENTIFIER_REGISTRY(tool);
	private:
		bool _load_tool(std::string_view tool_identifier, ast::NodeCPtr root);
	public:
		bool load_battleplannersettings_file(ast::NodeCPtr root);
	};
}