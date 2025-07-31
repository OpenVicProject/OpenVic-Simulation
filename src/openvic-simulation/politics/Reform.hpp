#pragma once

#include "openvic-simulation/politics/BaseIssue.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"

namespace OpenVic {	
	struct ReformGroup;

	// Reform type (i.e. political_issues)
	struct ReformType : HasIdentifier {
		friend struct IssueManager;

	private:
		bool PROPERTY(is_uncivilised); // whether this group is available to non-westernised countries
		// in vanilla education, military and economic reforms are hardcoded to true and the rest to false
		memory::vector<ReformGroup const*> PROPERTY(reform_groups);

	public:
		ReformType(std::string_view new_identifier, bool new_is_uncivilised);
		ReformType(ReformType&&) = default;
	};

	struct Reform;

	// Reform group (i.e. slavery)
	struct ReformGroup : HasIndex<ReformGroup>, BaseIssueGroup {

	private:
		ReformType const& PROPERTY(reform_type);
		const bool PROPERTY(is_ordered); // next_step_only
		const bool PROPERTY(is_administrative);

	public:
		ReformGroup(
			std::string_view new_identifier,
			index_t new_index,
			ReformType const& new_reform_type,
			bool new_is_ordered,
			bool new_is_administrative
		);
		ReformGroup(ReformGroup&&) = default;

		std::span<Reform const* const> get_reforms() const {
			return { reinterpret_cast<Reform const* const*>(get_issues().data()), get_issues().size() };
		}

		constexpr bool is_uncivilised() const {
			return reform_type.get_is_uncivilised();
		}
	};

	// Reform (i.e. yes_slavery)
	struct Reform : HasIndex<Reform>, BaseIssue {
		friend struct IssueManager;
		using tech_cost_t = uint32_t;

	private:
		const size_t PROPERTY(ordinal); // assigned by the parser to allow policy sorting
		const fixed_point_t PROPERTY(administrative_multiplier);
		const tech_cost_t PROPERTY(technology_cost);
		ConditionScript PROPERTY(allow);
		ConditionScript PROPERTY(on_execute_trigger);
		EffectScript PROPERTY(on_execute_effect);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		Reform(
			index_t new_index, std::string_view new_identifier,
			colour_t new_colour, ModifierValue&& new_values, ReformGroup const& new_reform_group,
			size_t new_ordinal, fixed_point_t new_administrative_multiplier,
			RuleSet&& new_rules, tech_cost_t new_technology_cost, ConditionScript&& new_allow,
			ConditionScript&& new_on_execute_trigger, EffectScript&& new_on_execute_effect
		);
		Reform(Reform&&) = default;

		constexpr ReformGroup const& get_reform_group() const {
			return static_cast<ReformGroup const&>(get_issue_group());
		}
	};
}