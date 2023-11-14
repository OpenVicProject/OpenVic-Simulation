#pragma once

#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"
#include "openvic-simulation/politics/NationalValue.hpp"
#include "openvic-simulation/politics/NationalFocus.hpp"

namespace OpenVic {
	struct PoliticsManager {
	private:
		GovernmentTypeManager government_type_manager;
		IdeologyManager ideology_manager;
		IssueManager issue_manager;
		NationalValueManager national_value_manager;
		NationalFocusManager national_focus_manager;

	public:
		REF_GETTERS(government_type_manager)
		REF_GETTERS(ideology_manager)
		REF_GETTERS(issue_manager)
		REF_GETTERS(national_value_manager)
		REF_GETTERS(national_focus_manager)

		inline bool load_government_types_file(ast::NodeCPtr root) {
			return government_type_manager.load_government_types_file(ideology_manager, root);
		}
		inline bool load_national_foci_file(PopManager const& pop_manager, GoodManager const& good_manager, ModifierManager const& modifier_manager, ast::NodeCPtr root) {
			return national_focus_manager.load_national_foci_file(pop_manager, ideology_manager, good_manager, modifier_manager, root);
		}
	};
}
