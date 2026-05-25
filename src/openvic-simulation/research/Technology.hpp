#pragma once

#include <cstdint>
#include <functional>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/types/UnitVariant.hpp"

namespace OpenVic {
	struct BuildingType;
	struct Technology;
	struct TechnologyArea;
	struct TechnologyManager;
	struct Invention;

	struct TechnologyFolder : HasIdentifier, HasIndex<TechnologyFolder, technology_folder_index_t> {
		friend struct TechnologyManager;

	private:
		memory::vector<std::reference_wrapper<const TechnologyArea>> SPAN_PROPERTY(technology_areas);

	public:
		TechnologyFolder(std::string_view new_identifier, index_t new_index);
		TechnologyFolder(TechnologyFolder&&) = default;
	};

	struct TechnologyArea : HasIdentifier {
		friend struct TechnologyManager;

	private:
		memory::vector<std::reference_wrapper<const Technology>> SPAN_PROPERTY(technologies);
		size_t PROPERTY(tech_count, 0);

	public:
		TechnologyFolder const& folder;

		TechnologyArea(std::string_view new_identifier, TechnologyFolder const& new_folder);
		TechnologyArea(TechnologyArea&&) = default;
	};

	struct Technology : Modifier, HasIndex<Technology, technology_index_t> {
		friend struct TechnologyManager;

		using area_index_t = uint8_t;
		using building_set_t = ordered_set<BuildingType const*>;

	private:
		std::optional<unit_variant_t> PROPERTY(unit_variant);
		ConditionalWeightFactorMul PROPERTY(ai_chance);
		memory::vector<std::reference_wrapper<const Invention>> PROPERTY(inventions);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		const Date::year_t year;
		TechnologyArea const& area;
		const fixed_point_t cost;
		const area_index_t index_in_area;
		const bool unciv_military;
		const ordered_set<RegimentType const*> activated_regiment_types;
		const ordered_set<ShipType const*> activated_ship_types;
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
			std::remove_const_t<decltype(activated_regiment_types)>&& new_activated_regiment_types,
			std::remove_const_t<decltype(activated_ship_types)>&& new_activated_ship_types,
			building_set_t&& new_activated_buildings,
			ModifierValue&& new_values,
			ConditionalWeightFactorMul&& new_ai_chance
		);
		Technology(Technology&&) = default;

		void add_invention(Invention const& invention) {
			inventions.emplace_back(invention);
		}
	};

	struct TechnologySchool : Modifier {
	public:
		TechnologySchool(std::string_view new_identifier, ModifierValue&& new_values);
	};
}
