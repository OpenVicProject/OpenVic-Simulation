#pragma once

#include <string_view>

#include "openvic-simulation/core/template/FunctionalConcepts.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"

namespace ovdl::v2script {
	class Parser;
}

namespace OpenVic {
	struct GoodDefinitionManager;
	struct ModifierManager;
	struct TerrainTypeManager;

	struct UnitTypeManager {
	private:
		IdentifierRegistry<RegimentType> IDENTIFIER_REGISTRY(regiment_type);
		IdentifierRegistry<ShipType> IDENTIFIER_REGISTRY(ship_type);

	public:
		constexpr UnitTypeManager() {}

		void reserve_all_unit_types(size_t size);
		void lock_all_unit_types();

		bool add_regiment_type(
			std::string_view identifier, UnitType::unit_type_args_t& unit_args,
			RegimentType::regiment_type_args_t const& regiment_type_args
		);
		bool add_ship_type(
			std::string_view identifier, UnitType::unit_type_args_t& unit_args,
			ShipType::ship_type_args_t const& ship_type_args
		);

		static Callback<std::string_view> auto expect_branch_str(
			Callback<unit_branch_t> auto callback
		) {
			using enum unit_branch_t;
			static const string_map_t<unit_branch_t> branch_map {
				{ "land", LAND }, { "naval", NAVAL }, { "sea", NAVAL }
			};
			return NodeTools::expect_mapped_string(branch_map, callback);
		}
		static NodeTools::NodeCallback auto expect_branch_identifier(Callback<unit_branch_t> auto callback) {
			return NodeTools::expect_identifier(expect_branch_str(callback));
		}

		bool load_unit_type_file(
			GoodDefinitionManager const& good_definition_manager, TerrainTypeManager const& terrain_type_manager,
			ModifierManager const& modifier_manager, ovdl::v2script::Parser const& parser
		);
		bool generate_modifiers(ModifierManager& modifier_manager) const;
	};
}
