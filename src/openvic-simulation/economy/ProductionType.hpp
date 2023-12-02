#pragma once

#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#define PRODUCTION_TYPE_ARGS \
	std::string_view identifier, EmployedPop owner, std::vector<EmployedPop> employees, ProductionType::type_t type, \
	Pop::pop_size_t workforce, Good::good_map_t&& input_goods, Good const* output_goods, fixed_point_t value, \
	std::vector<Bonus>&& bonuses, Good::good_map_t&& efficiency, bool coastal, bool farm, bool mine

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
			PopType const* pop_type, bool artisan, effect_t effect, fixed_point_t effect_multiplier, fixed_point_t amount
		);

	public:
		EmployedPop() = default;
	};

	struct Bonus {
		// TODO: trigger condition(s)
		const fixed_point_t value;
	};

	struct ProductionType : HasIdentifier {
		friend struct ProductionTypeManager;

	private:
		const EmployedPop PROPERTY(owner);
		const std::vector<EmployedPop> PROPERTY(employees);
		const enum struct type_t { FACTORY, RGO, ARTISAN } PROPERTY(type);
		const Pop::pop_size_t workforce;

		const Good::good_map_t PROPERTY(input_goods);
		Good const* PROPERTY(output_goods);
		const fixed_point_t PROPERTY(value);
		const std::vector<Bonus> PROPERTY(bonuses);

		const Good::good_map_t PROPERTY(efficiency);
		const bool PROPERTY_CUSTOM_PREFIX(coastal, is); // is_coastal

		const bool PROPERTY_CUSTOM_PREFIX(farm, is);
		const bool PROPERTY_CUSTOM_PREFIX(mine, is);

		ProductionType(PRODUCTION_TYPE_ARGS);

	public:
		ProductionType(ProductionType&&) = default;
	};

	struct ProductionTypeManager {
	private:
		IdentifierRegistry<ProductionType> production_types;
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

		bool add_production_type(PRODUCTION_TYPE_ARGS);
		IDENTIFIER_REGISTRY_ACCESSORS(production_type)

		bool load_production_types_file(GoodManager const& good_manager, PopManager const& pop_manager, ast::NodeCPtr root);
	};
}
