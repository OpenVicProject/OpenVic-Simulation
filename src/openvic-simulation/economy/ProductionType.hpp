#pragma once

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/pop/Pop.hpp"
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
		fixed_point_t PROPERTY(desired_workforce_share);

		Job(
			PopType const* new_pop_type,
			effect_t new_effect_type,
			fixed_point_t new_effect_multiplier,
			fixed_point_t new_desired_workforce_share
		);

	public:
		Job() = default;
	};

	struct ProductionType : HasIdentifier {
		friend struct ProductionTypeManager;

		enum struct template_type_t { FACTORY, RGO, ARTISAN };

		using bonus_t = std::pair<ConditionScript, fixed_point_t>;

	private:
		const std::optional<Job> PROPERTY(owner);
		std::vector<Job> PROPERTY(jobs);
		const template_type_t PROPERTY(template_type);
		const Pop::pop_size_t PROPERTY(base_workforce_size);

		GoodDefinition::good_definition_map_t PROPERTY(input_goods);
		GoodDefinition const* PROPERTY(output_goods);
		const fixed_point_t PROPERTY(base_output_quantity);
		std::vector<bonus_t> PROPERTY(bonuses);

		GoodDefinition::good_definition_map_t PROPERTY(maintenance_requirements);
		const bool PROPERTY_CUSTOM_PREFIX(coastal, is);

		const bool PROPERTY_CUSTOM_PREFIX(farm, is);
		const bool PROPERTY_CUSTOM_PREFIX(mine, is);

		ProductionType(
			std::string_view new_identifier,
			std::optional<Job> new_owner,
			std::vector<Job>&& new_jobs,
			template_type_t new_template_type,
			Pop::pop_size_t new_base_workforce_size,
			GoodDefinition::good_definition_map_t&& new_input_goods,
			GoodDefinition const* new_output_goods,
			fixed_point_t new_base_output_quantity,
			std::vector<bonus_t>&& new_bonuses,
			GoodDefinition::good_definition_map_t&& new_maintenance_requirements,
			bool new_is_coastal,
			bool new_is_farm,
			bool new_is_mine
		);

		bool parse_scripts(GameManager const& game_manager);

	public:
		ProductionType(ProductionType&&) = default;
	};

	struct ProductionTypeManager {
	private:
		IdentifierRegistry<ProductionType> IDENTIFIER_REGISTRY(production_type);
		PopType::sprite_t PROPERTY(rgo_owner_sprite);

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
			std::string_view identifier,
			std::optional<Job> owner,
			std::vector<Job>&& employees,
			ProductionType::template_type_t template_type,
			Pop::pop_size_t workforce,
			GoodDefinition::good_definition_map_t&& input_goods,
			GoodDefinition const* output_goods,
			fixed_point_t value,
			std::vector<ProductionType::bonus_t>&& bonuses,
			GoodDefinition::good_definition_map_t&& maintenance_requirements,
			bool coastal,
			bool farm,
			bool mine
		);

		bool load_production_types_file(
			GoodDefinitionManager const& good_definition_manager, PopManager const& pop_manager, ast::NodeCPtr root
		);

		bool parse_scripts(GameManager const& game_manager);
	};
}
