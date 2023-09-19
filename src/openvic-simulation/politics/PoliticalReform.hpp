#pragma once

#include <cstddef>
#include "types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct PoliticalReformManager;

	struct PoliticalReformGroup : HasIdentifier {
		friend struct PoliticalReformManager;

	private:
		PoliticalReformGroup(const std::string_view new_identifier, bool ordered);
		const bool ordered; //next_step_only, TODO default to false
	
	public:
		PoliticalReformGroup(PoliticalReformGroup&&) = default;
		bool is_ordered() const;
	};

	struct PoliticalReform : HasIdentifier {
		friend struct PoliticalReformManager;

	private:
		PoliticalReformGroup const& group;
		const size_t ordinal; //assigned by the parser to allow policy sorting

		//TODO - conditions to allow, policy modifiers, policy rule changes

		PoliticalReform(const std::string_view new_identifier, PoliticalReformGroup const& new_group, size_t ordinal);

	public:
		PoliticalReform(PoliticalReform&&) = default;
		size_t get_ordinal() const;
	};

	struct PoliticalReformManager {
	private:
		IdentifierRegistry<PoliticalReformGroup> political_reform_groups;
		IdentifierRegistry<PoliticalReform> political_reforms;

	public:
		PoliticalReformManager();
		
		bool add_political_reform_group(const std::string_view identifier, bool ordered);
		IDENTIFIER_REGISTRY_ACCESSORS(PoliticalReformGroup, political_reform_group)

		bool add_political_reform(const std::string_view identifier, PoliticalReformGroup const* group, size_t ordinal);
		IDENTIFIER_REGISTRY_ACCESSORS(PoliticalReform, political_reform)

		//TODO - loaders
	};
}