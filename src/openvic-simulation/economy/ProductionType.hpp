#pragma once

#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct ProductionTypeManager;

	struct EmployedPop {
		friend struct ProductionTypeManager;

		enum struct effect_t { INPUT, OUTPUT, THROUGHPUT };

	private:
		PopType const* PROPERTY(pop_type); // poptype
		bool PROPERTY(artisan); // set by the parser if the magic "artisan" poptype is passed
		effect_t PROPERTY(effect);
		fixed_point_t PROPERTY(effect_multiplier);
		fixed_point_t PROPERTY(amount);

		EmployedPop(
			PopType const* new_pop_type, bool new_artisan, effect_t new_effect, fixed_point_t new_effect_multiplier,
			fixed_point_t new_amount
		);

	public:
		EmployedPop() = default;
	};

	struct ProductionType : HasIdentifier {
		friend struct ProductionTypeManager;

		enum struct type_t { FACTORY, RGO, ARTISAN };

		using bonus_t = std::pair<ConditionScript, fixed_point_t>;

	private:
		const EmployedPop PROPERTY(owner);
		std::vector<EmployedPop> PROPERTY(employees);
		const type_t PROPERTY(type);
		const Pop::pop_size_t workforce;

		Good::good_map_t PROPERTY(input_goods);
		Good const* PROPERTY(output_goods);
		const fixed_point_t PROPERTY(value);
		std::vector<bonus_t> PROPERTY(bonuses);

		Good::good_map_t PROPERTY(efficiency);
		const bool PROPERTY_CUSTOM_PREFIX(coastal, is); // is_coastal

		const bool PROPERTY_CUSTOM_PREFIX(farm, is);
		const bool PROPERTY_CUSTOM_PREFIX(mine, is);

		ProductionType(
			std::string_view new_identifier, EmployedPop new_owner, std::vector<EmployedPop> new_employees, type_t new_type,
			Pop::pop_size_t new_workforce, Good::good_map_t&& new_input_goods, Good const* new_output_goods,
			fixed_point_t new_value, std::vector<bonus_t>&& new_bonuses, Good::good_map_t&& new_efficiency, bool new_coastal,
			bool new_farm, bool new_mine
		);

		bool parse_scripts(GameManager const& game_manager);
	public:
		ProductionType(ProductionType&&) = default;
	};

	struct ProductionTypeManager {
	private:
		IdentifierRegistry<ProductionType> IDENTIFIER_REGISTRY(production_type);
		PopType::sprite_t PROPERTY(rgo_owner_sprite);

		NodeTools::node_callback_t _expect_employed_pop(
			GoodManager const& good_manager, PopManager const& pop_manager, NodeTools::callback_t<EmployedPop&&> cb
		);
		NodeTools::node_callback_t _expect_employed_pop_list(
			GoodManager const& good_manager, PopManager const& pop_manager,
			NodeTools::callback_t<std::vector<EmployedPop>&&> cb
		);

	public:
		ProductionTypeManager();

		bool add_production_type(
			std::string_view identifier, EmployedPop owner, std::vector<EmployedPop> employees, ProductionType::type_t type,
			Pop::pop_size_t workforce, Good::good_map_t&& input_goods, Good const* output_goods, fixed_point_t value,
			std::vector<ProductionType::bonus_t>&& bonuses, Good::good_map_t&& efficiency, bool coastal, bool farm, bool mine
		);

		bool load_production_types_file(GoodManager const& good_manager, PopManager const& pop_manager, ast::NodeCPtr root);

		bool parse_scripts(GameManager const& game_manager);
	};
}
