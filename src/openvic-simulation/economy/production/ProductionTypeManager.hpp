#pragma once

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/core/stl/containers/TypedSpan.hpp"
#include "openvic-simulation/dataloader/NodeCallbacks.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/population/PopSize.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/PopSprite.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace ovdl::v2script {
	class Parser;
}

namespace OpenVic {
	struct ProductionTypeManager {
	private:
		IdentifierRegistry<ProductionType> IDENTIFIER_REGISTRY(production_type);
		pop_sprite_t PROPERTY(rgo_owner_sprite, 0);
		OV_IFLATMAP_PROPERTY(GoodDefinition, ProductionType const*, good_to_rgo_production_type);

		NodeTools::node_callback_t _expect_job(
			GoodDefinitionManager const& good_definition_manager, PopManager const& pop_manager,
			NodeTools::callback_t<
				pop_type_index_t,
				Job::effect_t,
				fixed_point_t,
				fixed_point_t
			> emplace_callback
		);
		NodeTools::node_callback_t _expect_job_list(
			GoodDefinitionManager const& good_definition_manager, PopManager const& pop_manager,
			NodeTools::callback_t<memory::vector<Job>&&> callback
		);

	public:
		constexpr ProductionTypeManager()
			: good_to_rgo_production_type { decltype(good_to_rgo_production_type){create_empty} } {}

		bool add_production_type(
			GameRulesManager const& game_rules_manager,
			TypedSpan<pop_type_index_t, const PopType> pop_types,
			const std::string_view identifier,
			std::optional<Job>&& owner,
			memory::vector<Job>&& jobs,
			const ProductionType::template_type_t template_type,
			const pop_size_t base_workforce_size,
			fixed_point_map_t<GoodDefinition const*>&& input_goods,
			GoodDefinition const* const output_good,
			const fixed_point_t base_output_quantity,
			memory::vector<ProductionType::bonus_t>&& bonuses,
			fixed_point_map_t<GoodDefinition const*>&& maintenance_requirements,
			const bool is_coastal,
			const bool is_farm,
			const bool is_mine
		);

		bool load_production_types_file(
			GameRulesManager const& game_rules_manager,
			GoodDefinitionManager const& good_definition_manager,
			PopManager const& pop_manager,
			ovdl::v2script::Parser const& parser
		);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}