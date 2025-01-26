#pragma once

#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/pop/PopType.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct ProductionTypeManager;

	struct Job {
		friend struct ProductionTypeManager;

		enum struct effect_t { INPUT, OUTPUT, THROUGHPUT };

	private:
		PopType const* PROPERTY(pop_type);
		effect_t PROPERTY(effect_type);
		fixed_point_t PROPERTY(effect_multiplier);
		fixed_point_t PROPERTY(amount);

		Job(
			PopType const* new_pop_type,
			effect_t new_effect_type,
			fixed_point_t new_effect_multiplier,
			fixed_point_t new_amount
		);

	public:
		Job() = default;
	};

	struct ProductionType : HasIdentifier {
		friend struct ProductionTypeManager;

		enum struct template_type_t { FACTORY, RGO, ARTISAN };

		using bonus_t = std::pair<ConditionScript, fixed_point_t>;

	private:
		GameRulesManager const& game_rules_manager;
		const std::optional<Job> PROPERTY(owner);
		std::vector<Job> PROPERTY(jobs);
		const template_type_t PROPERTY(template_type);
		const pop_size_t PROPERTY(base_workforce_size);

		GoodDefinition::good_definition_map_t PROPERTY(input_goods);
		GoodDefinition const& PROPERTY(output_good);
		const fixed_point_t PROPERTY(base_output_quantity);
		std::vector<bonus_t> PROPERTY(bonuses);

		GoodDefinition::good_definition_map_t PROPERTY(maintenance_requirements);
		const bool PROPERTY_CUSTOM_PREFIX(coastal, is);
		const bool _is_farm, _is_mine;

		ProductionType(
			GameRulesManager const& new_game_rules_manager,
			const std::string_view new_identifier,
			const std::optional<Job> new_owner,
			std::vector<Job>&& new_jobs,
			const template_type_t new_template_type,
			const pop_size_t new_base_workforce_size,
			GoodDefinition::good_definition_map_t&& new_input_goods,
			GoodDefinition const& new_output_good,
			const fixed_point_t new_base_output_quantity,
			std::vector<bonus_t>&& new_bonuses,
			GoodDefinition::good_definition_map_t&& new_maintenance_requirements,
			const bool new_is_coastal,
			const bool new_is_farm,
			const bool new_is_mine
		);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		constexpr bool get_is_farm_for_tech() const {
			if (game_rules_manager.get_use_simple_farm_mine_logic()) {
				return _is_farm;
			}

			return !_is_mine && _is_farm;
		}
		constexpr bool get_is_farm_for_non_tech() const {
			return _is_farm;
		}
		constexpr bool get_is_mine_for_tech() const {
			return _is_mine;
		}
		constexpr bool get_is_mine_for_non_tech() const {
			if (game_rules_manager.get_use_simple_farm_mine_logic()) {
				return _is_mine;
			}

			return !_is_farm;
		}
		ProductionType(ProductionType&&) = default;
	};

	struct ProductionTypeManager {
	private:
		IdentifierRegistry<ProductionType> IDENTIFIER_REGISTRY(production_type);
		PopType::sprite_t PROPERTY(rgo_owner_sprite);
		IndexedMap<GoodDefinition, ProductionType const*> PROPERTY(good_to_rgo_production_type);

		NodeTools::node_callback_t _expect_job(
			GoodDefinitionManager const& good_definition_manager, PopManager const& pop_manager,
			NodeTools::callback_t<Job&&> callback
		);
		NodeTools::node_callback_t _expect_job_list(
			GoodDefinitionManager const& good_definition_manager, PopManager const& pop_manager,
			NodeTools::callback_t<std::vector<Job>&&> callback
		);

	public:
		ProductionTypeManager();

		bool add_production_type(
			GameRulesManager const& game_rules_manager,
			const std::string_view identifier,
			std::optional<Job> owner,
			std::vector<Job>&& jobs,
			const ProductionType::template_type_t template_type,
			const pop_size_t base_workforce_size,
			GoodDefinition::good_definition_map_t&& input_goods,
			GoodDefinition const* const output_good,
			const fixed_point_t base_output_quantity,
			std::vector<ProductionType::bonus_t>&& bonuses,
			GoodDefinition::good_definition_map_t&& maintenance_requirements,
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
