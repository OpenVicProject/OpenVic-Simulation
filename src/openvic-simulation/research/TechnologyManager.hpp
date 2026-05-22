#pragma once

#include <string_view>

#include "openvic-simulation/dataloader/Node_forwarded.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/research/Technology.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/UnitVariant.hpp"

namespace OpenVic {
	struct BuildingTypeManager;
	struct TechnologyManager;
	struct UnitTypeManager;

	struct TechnologyManager {
		friend struct InventionManager;

		IdentifierRegistry<TechnologyFolder> IDENTIFIER_REGISTRY(technology_folder);
		IdentifierRegistry<TechnologyArea> IDENTIFIER_REGISTRY(technology_area);
		IdentifierRegistry<Technology> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(technology, technologies);
		IdentifierRegistry<TechnologySchool> IDENTIFIER_REGISTRY(technology_school);

		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_PLURAL(technology, technologies);

	public:
		bool add_technology_folder(std::string_view identifier);

		bool add_technology_area(std::string_view identifier, TechnologyFolder const& folder);

		bool add_technology(
			std::string_view identifier, TechnologyArea* area, Date::year_t year, fixed_point_t cost, bool unciv_military,
			std::optional<unit_variant_t>&& unit_variant, Technology::unit_set_t&& activated_units,
			Technology::building_set_t&& activated_buildings, ModifierValue&& values, ConditionalWeightFactorMul&& ai_chance
		);

		bool add_technology_school(std::string_view identifier, ModifierValue&& values);

		/* Both of these functions load data from "common/technology.txt", they are separated because the schools depend
		 * on modifiers generated from the folder definitions and so loading must be staggered. */
		bool load_technology_file_folders_and_areas(ovdl::v2script::ast::Node const* root);
		bool load_technology_file_schools(ModifierManager const& modifier_manager, ovdl::v2script::ast::Node const* root);

		/* Loaded from "technologies/.txt" files named after technology folders. */
		bool load_technologies_file(
			ModifierManager const& modifier_manager, UnitTypeManager const& unit_type_manager,
			BuildingTypeManager const& building_type_manager, ovdl::v2script::ast::Node const* root
		);

		bool generate_modifiers(ModifierManager& modifier_manager) const;

		/* Populates the lists of technology areas for each technology folder and technologies for each technology area. */
		bool generate_technology_lists();

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
