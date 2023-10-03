#pragma once

#include "misc/Modifier.hpp"
#include "types/Date.hpp"
#include "types/IdentifierRegistry.hpp"
#include "types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct TechnologyFolder : HasIdentifier {
		friend struct TechnologyManager;

	private:
		TechnologyFolder(std::string_view identifier);

	public:
		TechnologyFolder(TechnologyFolder&&) = default;
	};

	struct TechnologyArea : HasIdentifier {
		friend struct TechnologyManager;

	private:
		TechnologyFolder const& PROPERTY(folder);

		TechnologyArea(std::string_view identifier, TechnologyFolder const& folder);

	public:
		TechnologyArea(TechnologyArea&&) = default;
	};

	struct Technology : Modifier {
		friend struct TechnologyManager;
		using year_t = Date::year_t;

	private:
		TechnologyArea const& PROPERTY(area);
		const year_t PROPERTY(year);
		const fixed_point_t PROPERTY(cost);
		const bool PROPERTY(unciv_military);

		//TODO: implement rules/modifiers and ai_chance

		Technology(std::string_view identifier, TechnologyArea const& area, year_t year, fixed_point_t cost, bool unciv_military, ModifierValue&& values);
	
	public:
		Technology(Technology&&) = default;
	};

	struct TechnologySchool : Modifier {
		friend struct TechnologyManager;

	private:
		TechnologySchool(std::string_view new_identifier, ModifierValue&& new_values);
	};

	struct TechnologyManager {
		IdentifierRegistry<TechnologyFolder> technology_folders;
		IdentifierRegistry<TechnologyArea> technology_areas;
		IdentifierRegistry<Technology> technologies;
		IdentifierRegistry<TechnologySchool> technology_schools;

	public:
		TechnologyManager();

		bool add_technology_folder(std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(technology_folder)

		bool add_technology_area(std::string_view identifier, TechnologyFolder const* folder);
		IDENTIFIER_REGISTRY_ACCESSORS(technology_area)

		bool add_technology(std::string_view identifier, TechnologyArea const* area, Technology::year_t year, fixed_point_t cost, bool unciv_military, ModifierValue&& values);
		IDENTIFIER_REGISTRY_ACCESSORS_CUSTOM_PLURAL(technology, technologies)

		bool add_technology_school(std::string_view identifier, ModifierValue&& values); //, Modifier::icon_t icon);
		IDENTIFIER_REGISTRY_ACCESSORS(technology_school);

		bool load_technology_file(ModifierManager const& modifier_manager, ast::NodeCPtr root); // common/technology.txt
		bool load_technologies_file(ModifierManager const& modifier_manager, ast::NodeCPtr root); // technologies/*.txt
	};
}