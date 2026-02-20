#pragma once

#include <cstdint>

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/types/UnitVariant.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct BuildingType;
	struct BuildingTypeManager;
	struct Technology;
	struct TechnologyArea;
	struct TechnologyManager;
	struct UnitTypeManager;
	struct Invention;

	struct TechnologyFolder : HasIdentifier, HasIndex<TechnologyFolder, technology_folder_index_t> {
		friend struct TechnologyManager;

	private:
		memory::vector<TechnologyArea const*> SPAN_PROPERTY(technology_areas);

	public:
		TechnologyFolder(std::string_view new_identifier, index_t new_index);
		TechnologyFolder(TechnologyFolder&&) = default;
	};

	struct TechnologyArea : HasIdentifier {
		friend struct TechnologyManager;

	private:
		memory::vector<Technology const*> SPAN_PROPERTY(technologies);
		size_t PROPERTY(tech_count, 0);

	public:
		TechnologyFolder const& folder;

		TechnologyArea(std::string_view new_identifier, TechnologyFolder const& new_folder);
		TechnologyArea(TechnologyArea&&) = default;
	};

	struct Technology : Modifier, HasIndex<Technology, technology_index_t> {
		friend struct TechnologyManager;

		using area_index_t = uint8_t;
		using unit_set_t = ordered_set<UnitType const*>;
		using building_set_t = ordered_set<BuildingType const*>;

	private:
		std::optional<unit_variant_t> PROPERTY(unit_variant);
		ConditionalWeightFactorMul PROPERTY(ai_chance);
		memory::vector<Invention const*> PROPERTY(inventions);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		const Date::year_t year;
		TechnologyArea const& area;
		const fixed_point_t cost;
		const area_index_t index_in_area;
		const bool unciv_military;
		const unit_set_t activated_units;
		const building_set_t activated_buildings;

		Technology(
			std::string_view new_identifier,
			index_t new_index,
			TechnologyArea const& new_area,
			Date::year_t new_year,
			fixed_point_t new_cost,
			area_index_t new_index_in_area,
			bool new_unciv_military,
			std::optional<unit_variant_t>&& new_unit_variant,
			unit_set_t&& new_activated_units,
			building_set_t&& new_activated_buildings,
			ModifierValue&& new_values,
			ConditionalWeightFactorMul&& new_ai_chance
		);
		Technology(Technology&&) = default;

		void add_invention(Invention const* invention) {
			inventions.push_back(invention);
		}
	};

	struct TechnologySchool : Modifier {
	public:
		TechnologySchool(std::string_view new_identifier, ModifierValue&& new_values);
	};

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
		bool load_technology_file_folders_and_areas(ast::NodeCPtr root);
		bool load_technology_file_schools(ModifierManager const& modifier_manager, ast::NodeCPtr root);

		/* Loaded from "technologies/.txt" files named after technology folders. */
		bool load_technologies_file(
			ModifierManager const& modifier_manager, UnitTypeManager const& unit_type_manager,
			BuildingTypeManager const& building_type_manager, ast::NodeCPtr root
		);

		bool generate_modifiers(ModifierManager& modifier_manager) const;

		/* Populates the lists of technology areas for each technology folder and technologies for each technology area. */
		bool generate_technology_lists();

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
