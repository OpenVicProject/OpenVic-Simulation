#pragma once

#include "Rebel.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"
#include "openvic-simulation/politics/NationalValue.hpp"
#include "openvic-simulation/politics/NationalFocus.hpp"

namespace OpenVic {
	struct PoliticsManager {
	private:
		GovernmentTypeManager PROPERTY_REF(government_type_manager);
		IdeologyManager PROPERTY_REF(ideology_manager);
		IssueManager PROPERTY_REF(issue_manager);
		NationalValueManager PROPERTY_REF(national_value_manager);
		NationalFocusManager PROPERTY_REF(national_focus_manager);
		RebelManager PROPERTY_REF(rebel_manager);

	public:
		inline bool load_government_types_file(ast::NodeCPtr root) {
			return government_type_manager.load_government_types_file(ideology_manager, root);
		}
		inline bool load_national_foci_file(PopManager const& pop_manager, GoodManager const& good_manager, ModifierManager const& modifier_manager, ast::NodeCPtr root) {
			return national_focus_manager.load_national_foci_file(pop_manager, ideology_manager, good_manager, modifier_manager, root);
		}
	};
}
