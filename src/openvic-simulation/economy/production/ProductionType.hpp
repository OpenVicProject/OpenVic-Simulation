#pragma once

#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/types/PopSprite.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct GameRulesManager;
	struct GoodDefinition;
	struct GoodDefinitionManager;
	struct PopManager;
	struct PopType;
	struct ProductionTypeManager;

	struct Job {
		friend struct ProductionTypeManager;

		enum struct effect_t { THROUGHPUT, INPUT, OUTPUT };

	private:
		PopType const* PROPERTY(pop_type, nullptr);
		effect_t PROPERTY(effect_type, effect_t::THROUGHPUT);
		fixed_point_t PROPERTY(effect_multiplier, 0);
		fixed_point_t PROPERTY(amount, 0);

		Job(
			PopType const* new_pop_type,
			effect_t new_effect_type,
			fixed_point_t new_effect_multiplier,
			fixed_point_t new_amount
		);

	public:
		constexpr Job() {};
	};

	struct ProductionType : HasIdentifier {
		friend struct ProductionTypeManager;

		enum struct template_type_t { FACTORY, RGO, ARTISAN };

		using bonus_t = std::pair<ConditionScript, fixed_point_t>;

	private:
		GameRulesManager const& game_rules_manager;
		const std::optional<Job> PROPERTY(owner);
		memory::vector<Job> PROPERTY(jobs);
		const template_type_t PROPERTY(template_type);
		const pop_size_t PROPERTY(base_workforce_size);

		fixed_point_map_t<GoodDefinition const*> PROPERTY(input_goods);
		GoodDefinition const& PROPERTY(output_good);
		const fixed_point_t PROPERTY(base_output_quantity);
		memory::vector<bonus_t> PROPERTY(bonuses);

		fixed_point_map_t<GoodDefinition const*> PROPERTY(maintenance_requirements);
		const bool PROPERTY_CUSTOM_PREFIX(coastal, is);
		const bool _is_farm, _is_mine;
		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		ProductionType(
			GameRulesManager const& new_game_rules_manager,
			const std::string_view new_identifier,
			const std::optional<Job> new_owner,
			memory::vector<Job>&& new_jobs,
			const template_type_t new_template_type,
			const pop_size_t new_base_workforce_size,
			fixed_point_map_t<GoodDefinition const*>&& new_input_goods,
			GoodDefinition const& new_output_good,
			const fixed_point_t new_base_output_quantity,
			memory::vector<bonus_t>&& new_bonuses,
			fixed_point_map_t<GoodDefinition const*>&& new_maintenance_requirements,
			const bool new_is_coastal,
			const bool new_is_farm,
			const bool new_is_mine
		);

		constexpr bool get_is_farm_for_non_tech() const {
			return _is_farm;
		}
		constexpr bool get_is_mine_for_tech() const {
			return _is_mine;
		}
		bool get_is_farm_for_tech() const;
		bool get_is_mine_for_non_tech() const;
		ProductionType(ProductionType&&) = default;
	};

	struct ProductionTypeManager {
	private:
		IdentifierRegistry<ProductionType> IDENTIFIER_REGISTRY(production_type);
		pop_sprite_t PROPERTY(rgo_owner_sprite);
		OV_IFLATMAP_PROPERTY(GoodDefinition, ProductionType const*, good_to_rgo_production_type);

		NodeTools::node_callback_t _expect_job(
			GoodDefinitionManager const& good_definition_manager, PopManager const& pop_manager,
			NodeTools::callback_t<Job&&> callback
		);
		NodeTools::node_callback_t _expect_job_list(
			GoodDefinitionManager const& good_definition_manager, PopManager const& pop_manager,
			NodeTools::callback_t<memory::vector<Job>&&> callback
		);

	public:
		constexpr ProductionTypeManager()
			: rgo_owner_sprite { 0 },
			good_to_rgo_production_type { decltype(good_to_rgo_production_type)::create_empty() } {}

		bool add_production_type(
			GameRulesManager const& game_rules_manager,
			const std::string_view identifier,
			std::optional<Job> owner,
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