#pragma once

#include <string_view>
#include <vector>
#include <set>
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-dataloader/v2script/AbstractSyntaxTree.hpp"

#define ARGS(enum_type, output) std::string_view identifier, EmployedPop owner, std::vector<EmployedPop> employees, enum_type type, \
								uint32_t workforce, std::map<const Good*, fixed_point_t> input_goods, output output_goods, \
								fixed_point_t value, std::vector<Bonus> bonuses, std::map<const Good*, fixed_point_t> efficiency, \
								bool coastal, bool farm, bool mine

namespace OpenVic {
	struct ProductionTypeManager;

	struct EmployedPop {
		friend struct ProductionTypeManager;

	private:
		PopType const* pop_type; //poptype
		enum struct effect_t {
			INPUT,
			OUTPUT,
			THROUGHPUT
		} effect;
		fixed_point_t effect_multiplier;
		fixed_point_t amount;

		EmployedPop(PopType const* pop_type, effect_t effect, fixed_point_t effect_multiplier, fixed_point_t amount);
	
	public:
		EmployedPop() = default; 
	
		PopType const* get_pop_type();
		effect_t get_effect();
		fixed_point_t get_effect_multiplier();
		fixed_point_t get_amount();
	};

	struct Bonus {
		//TODO: trigger condition(s)
		const fixed_point_t value;
	};

	struct ProductionType : HasIdentifier {
		friend struct ProductionTypeManager;

	private:
		const EmployedPop owner;
		const std::vector<EmployedPop> employees;
		const enum struct type_t {
			FACTORY,
			RGO,
			ARTISAN
		} type;
		const uint32_t workforce;

		const std::map<const Good*, fixed_point_t> input_goods; //farms generally lack this
		const Good* output_goods;
		const fixed_point_t value;
		const std::vector<Bonus> bonuses; //some

		const std::map<const Good*, fixed_point_t> efficiency; //some
		const bool coastal; //is_coastal some(false)
		
		const bool farm; //some (false)
		const bool mine; //some (false)

		ProductionType(ARGS(type_t, const Good*));

	public:
		ProductionType(ProductionType&&) = default;

		EmployedPop const& get_owner() const;
		std::vector<EmployedPop> const& get_employees() const;
		type_t get_type() const;
		uint32_t get_workforce() const;

		std::map<const Good*, fixed_point_t> const& get_input_goods();
		const Good* get_output_goods() const;
		fixed_point_t get_value() const;
		std::vector<Bonus> const& get_bonuses();

		std::map<const Good*, fixed_point_t> const& get_efficiency();
		bool is_coastal() const;

		bool is_farm() const;
		bool is_mine() const;
	};

	struct ProductionTypeManager {
	private:
		IdentifierRegistry<ProductionType> production_types;

		NodeTools::node_callback_t _expect_employed_pop(GoodManager& good_manager, PopManager& pop_manager, 
			NodeTools::callback_t<EmployedPop> cb);
		NodeTools::node_callback_t _expect_employed_pop_list(GoodManager& good_manager, PopManager& pop_manager, 
			NodeTools::callback_t<std::vector<EmployedPop>> cb);
	
	public:
		ProductionTypeManager();

		bool add_production_type(ARGS(std::string_view, std::string_view), GoodManager& good_manager);
		IDENTIFIER_REGISTRY_ACCESSORS(ProductionType, production_type)

		bool load_production_types_file(GoodManager& good_manager, PopManager& pop_manager, ast::NodeCPtr root);
	};
}