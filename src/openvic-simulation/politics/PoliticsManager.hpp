#pragma once

#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/IssueManager.hpp"
#include "openvic-simulation/politics/NationalFocus.hpp"
#include "openvic-simulation/politics/NationalValue.hpp"
#include "openvic-simulation/politics/Rebel.hpp"
#include "openvic-simulation/politics/Rule.hpp"

namespace OpenVic {
	struct PoliticsManager {
	private:
		GovernmentTypeManager PROPERTY_REF(government_type_manager);
		IdeologyManager PROPERTY_REF(ideology_manager);
		RuleManager PROPERTY_REF(rule_manager);
		IssueManager PROPERTY_REF(issue_manager);
		NationalValueManager PROPERTY_REF(national_value_manager);
		NationalFocusManager PROPERTY_REF(national_focus_manager);
		RebelManager PROPERTY_REF(rebel_manager);

	public:
		inline bool load_government_types_file(ast::NodeCPtr root) {
			return government_type_manager.load_government_types_file(ideology_manager, root);
		}
		inline bool load_national_foci_file(
			PopManager const& pop_manager, GoodDefinitionManager const& good_definition_manager,
			ModifierManager const& modifier_manager, ast::NodeCPtr root
		) {
			return national_focus_manager.load_national_foci_file(
				pop_manager, ideology_manager, good_definition_manager, modifier_manager, root
			);
		}
		inline bool load_rebels_file(ast::NodeCPtr root) {
			return rebel_manager.load_rebels_file(ideology_manager, government_type_manager, root);
		}
		inline bool load_issues_file(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
			return issue_manager.load_issues_file(modifier_manager, rule_manager, root);
		}
	};
}
