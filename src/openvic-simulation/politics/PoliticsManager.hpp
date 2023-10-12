#pragma once

#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"

namespace OpenVic {
	struct PoliticsManager {
	private:
		GovernmentTypeManager government_type_manager;
		IdeologyManager ideology_manager;
		IssueManager issue_manager;
	public:
		REF_GETTERS(government_type_manager)
		REF_GETTERS(ideology_manager)
		REF_GETTERS(issue_manager)

		inline bool load_government_types_file(ast::NodeCPtr root) {
			return government_type_manager.load_government_types_file(ideology_manager, root);
		}
	};
}
