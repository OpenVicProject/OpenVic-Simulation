#pragma once

#include <utility>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/population/PopSize.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct PopType;

	struct Job {
		enum struct effect_t { THROUGHPUT, INPUT, OUTPUT };

	public:
		const pop_type_index_t pop_type_index;
		const effect_t effect_type;
		const fixed_point_t effect_multiplier;
		const fixed_point_t amount;
		
		constexpr Job(
			const pop_type_index_t new_pop_type_index,
			const effect_t new_effect_type,
			const fixed_point_t new_effect_multiplier,
			const fixed_point_t new_amount
		) : pop_type_index { new_pop_type_index },
			effect_type { new_effect_type },
			effect_multiplier { new_effect_multiplier },
			amount { new_amount } {}

		constexpr Job() : Job({}, effect_t::THROUGHPUT, fixed_point_t::_0, fixed_point_t::_0) {};
	};

	struct GameRulesManager;
	struct GoodDefinition;
	struct GoodDefinitionManager;
	struct PopManager;
	struct ProvinceInstance;
	struct State;

	struct ProductionType : HasIdentifier {
		friend struct ProductionTypeManager;

		enum struct template_type_t { FACTORY, RGO, ARTISAN };

		using bonus_t = std::pair<ConditionScript, fixed_point_t>;

	private:
		GameRulesManager const& game_rules_manager;
		const memory::vector<Job> SPAN_PROPERTY(jobs);

		memory::vector<bonus_t> SPAN_PROPERTY(bonuses);

		const bool _is_farm, _is_mine, is_coastal;
		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		const std::optional<Job> owner;
		const fixed_point_map_t<GoodDefinition const*> input_goods;
		const fixed_point_map_t<GoodDefinition const*> maintenance_requirements;
		GoodDefinition const& output_good;
		const template_type_t template_type;
		const pop_size_t base_workforce_size;
		const fixed_point_t base_output_quantity;

		ProductionType(
			GameRulesManager const& new_game_rules_manager,
			const std::string_view new_identifier,
			std::optional<Job>&& new_owner,
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
		bool is_valid_for_artisan_in(ProvinceInstance& province) const;
		bool is_valid_for_factory_in(State& state) const;
		ProductionType(ProductionType&&) = default;
	};
}