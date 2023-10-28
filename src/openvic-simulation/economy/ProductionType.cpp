#include "ProductionType.hpp"

#include <set>

using namespace OpenVic;
using namespace OpenVic::NodeTools;

EmployedPop::EmployedPop(PopType const* pop_type, bool artisan, effect_t effect,
	fixed_point_t effect_multiplier, fixed_point_t amount)
	: pop_type { pop_type }, artisan { artisan }, effect { effect },
	  effect_multiplier { effect_multiplier }, amount { amount } {}

PopType const* EmployedPop::get_pop_type() const {
	return pop_type;
}

bool EmployedPop::is_artisan() const {
	return artisan;
}

EmployedPop::effect_t EmployedPop::get_effect() const {
	return effect;
}

fixed_point_t EmployedPop::get_effect_multiplier() const {
	return effect_multiplier;
}

fixed_point_t EmployedPop::get_amount() const {
	return amount;
}

ProductionType::ProductionType(PRODUCTION_TYPE_ARGS) : HasIdentifier { identifier }, owner { owner }, employees { employees },
	type { type }, workforce { workforce }, input_goods { std::move(input_goods) }, output_goods { output_goods },
	value { value }, bonuses { std::move(bonuses) }, efficiency { std::move(efficiency) }, coastal { coastal }, farm { farm }, mine { mine } {}

EmployedPop const& ProductionType::get_owner() const {
	return owner;
}

std::vector<EmployedPop> const& ProductionType::get_employees() const {
	return employees;
}

ProductionType::type_t ProductionType::get_type() const {
	return type;
}

Pop::pop_size_t ProductionType::get_workforce() const {
	return workforce;
}

Good::good_map_t const& ProductionType::get_input_goods() const {
	return input_goods;
}

Good const* ProductionType::get_output_goods() const {
	return output_goods;
}

fixed_point_t ProductionType::get_value() const {
	return value;
}

std::vector<Bonus> const& ProductionType::get_bonuses() const {
	return bonuses;
}

Good::good_map_t const& ProductionType::get_efficiency() const {
	return efficiency;
}

bool ProductionType::is_coastal() const {
	return coastal;
}

bool ProductionType::is_farm() const {
	return farm;
}

bool ProductionType::is_mine() const {
	return mine;
}

ProductionTypeManager::ProductionTypeManager() : production_types { "production types" } {}

node_callback_t ProductionTypeManager::_expect_employed_pop(GoodManager const& good_manager, PopManager const& pop_manager,
	callback_t<EmployedPop&&> cb) {

	return [this, &good_manager, &pop_manager, cb](ast::NodeCPtr node) -> bool {
		std::string_view pop_type;
		EmployedPop::effect_t effect;
		fixed_point_t effect_multiplier = 1, amount = 1;

		using enum EmployedPop::effect_t;
		static const string_map_t<EmployedPop::effect_t> effect_map = {
			{ "input", INPUT }, { "output", OUTPUT }, { "throughput", THROUGHPUT }
		};

		bool res = expect_dictionary_keys(
			"poptype", ONE_EXACTLY, expect_identifier(assign_variable_callback(pop_type)),
			"effect", ONE_EXACTLY, expect_identifier(expect_mapped_string(effect_map, assign_variable_callback(effect))),
			"effect_multiplier", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(effect_multiplier)),
			"amount", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(amount))
		)(node);

		const PopType* found_pop_type = pop_manager.get_pop_type_by_identifier(pop_type);
		bool artisan = false;
		if (found_pop_type == nullptr) {
			if (pop_type == "artisan") {
				artisan = true;
			} else {
				Logger::error("Found invalid pop type ", pop_type, " while parsing production types!");
				return false;
			}
		}

		return res & cb({ found_pop_type, artisan, effect, effect_multiplier, amount });
	};
}

node_callback_t ProductionTypeManager::_expect_employed_pop_list(GoodManager const& good_manager, PopManager const& pop_manager,
	callback_t<std::vector<EmployedPop>&&> cb) {

	return [this, &good_manager, &pop_manager, cb](ast::NodeCPtr node) -> bool {
		std::vector<EmployedPop> employed_pops;
		bool res = expect_list([this, &good_manager, &pop_manager, &employed_pops](ast::NodeCPtr node) -> bool {
			EmployedPop owner;
			bool res_partial = _expect_employed_pop(good_manager, pop_manager, assign_variable_callback(owner))(node);
			employed_pops.push_back(owner);
			return res_partial;
		})(node);
		return res & cb(std::move(employed_pops));
	};
}

#define POPTYPE_CHECK(employed_pop) \
	if ((employed_pop.pop_type == nullptr && !employed_pop.artisan) || (employed_pop.pop_type != nullptr && employed_pop.artisan)) { \
		Logger::error("Invalid pop type parsed for owner of production type ", identifier, "!"); \
		return false; \
	}

bool ProductionTypeManager::add_production_type(PRODUCTION_TYPE_ARGS, GoodManager const& good_manager) {
	if (identifier.empty()) {
		Logger::error("Invalid production type identifier - empty!");
		return false;
	}

	if (workforce <= 0) {
		Logger::error("Workforce for production type ", identifier, " was 0 or unset!");
		return false;
	}

	if (value <= 0) {
		Logger::error("Value for production type ", identifier, " was 0 or unset!");
		return false;
	}

	POPTYPE_CHECK(owner)

	for (EmployedPop const& ep : employees) {
		POPTYPE_CHECK(ep)
	}

	if (output_goods == nullptr) {
		Logger::error("Output good for production type ", identifier, " was null!");
		return false;
	}

	return production_types.add_item({
		identifier, owner, employees, type, workforce, std::move(input_goods),
		output_goods, value, std::move(bonuses), std::move(efficiency), coastal, farm, mine
	});
}

#define PARSE_NODE expect_dictionary_keys_and_default( \
		key_value_success_callback, \
		"owner", ZERO_OR_ONE, _expect_employed_pop(good_manager, pop_manager, move_variable_callback(owner)), \
		"employees", ZERO_OR_ONE, _expect_employed_pop_list(good_manager, pop_manager, move_variable_callback(employees)), \
		"type", ZERO_OR_ONE, expect_identifier(expect_mapped_string(type_map, assign_variable_callback(type))), \
		"workforce", ZERO_OR_ONE, expect_uint(assign_variable_callback(workforce)), \
		"input_goods", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(input_goods)), \
		"output_goods", ZERO_OR_ONE, good_manager.expect_good_identifier(assign_variable_callback_pointer(output_goods)), \
		"value", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(value)), \
		"efficiency", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(efficiency)), \
		"is_coastal", ZERO_OR_ONE, expect_bool(assign_variable_callback(coastal)), \
		"farm", ZERO_OR_ONE, expect_bool(assign_variable_callback(farm)), \
		"mine", ZERO_OR_ONE, expect_bool(assign_variable_callback(mine)) \
	)

bool ProductionTypeManager::load_production_types_file(GoodManager const& good_manager,
	PopManager const& pop_manager, ast::NodeCPtr root) {
	size_t expected_types = 0;

	// pass 1: find and store template identifiers
	std::set<std::string_view> templates;
	std::map<std::string_view, std::string_view> template_target_map;
	bool ret = expect_dictionary(
		[this, &expected_types, &templates, &template_target_map](std::string_view key, ast::NodeCPtr value) -> bool {
			expected_types++;

			std::string_view template_id = "";
			bool found_template = false;
			const bool ret = expect_key("template", expect_identifier(assign_variable_callback(template_id)), &found_template)(value);
			if (found_template) {
				if (ret) {
					templates.emplace(template_id);
					template_target_map.emplace(key, template_id);
				} else {
					Logger::error("Failed get template identifier for ", key);
					return false;
				}
			}
			return true;
		}
	)(root);

	// pass 2: create and populate the template map
	std::map<std::string_view, ast::NodeCPtr> template_node_map;
	ret &= expect_dictionary(
		[this, &expected_types, &templates, &template_node_map](std::string_view key, ast::NodeCPtr value) -> bool {
			if (templates.contains(key)) {
				template_node_map.emplace(key, value);
				expected_types--;
			}
			return true;
		}
	)(root);

	// pass 3: actually load production types
	production_types.reserve(production_types.size() + expected_types);
	ret &= expect_dictionary(
		[this, &good_manager, &pop_manager, &template_target_map, &template_node_map](
			std::string_view key, ast::NodeCPtr node) -> bool {
			if (template_node_map.contains(key))
				return true;

			EmployedPop owner;
			std::vector<EmployedPop> employees;
			ProductionType::type_t type;
			Good const* output_goods = nullptr;
			Pop::pop_size_t workforce = 0; // 0 is a meaningless value -> unset
			Good::good_map_t input_goods, efficiency;
			fixed_point_t value = 0; // 0 is a meaningless value -> unset
			std::vector<Bonus> bonuses;
			bool coastal = false, farm = false, mine = false;

			bool ret = true;

			using enum ProductionType::type_t;
			static const string_map_t<ProductionType::type_t> type_map = {
				{ "factory", FACTORY }, { "rgo", RGO }, { "artisan", ARTISAN }
			};

			// apply template first
			if (template_target_map.contains(key)) {
				std::string_view template_id = template_target_map[key];
				if (template_node_map.contains(template_id)) {
					ast::NodeCPtr template_node = template_node_map[template_id];
					ret &= PARSE_NODE(template_node);
				}
			}

			ret &= PARSE_NODE(node);

			ret &= add_production_type(
				key, owner, employees, type, workforce, std::move(input_goods), output_goods, value,
				std::move(bonuses), std::move(efficiency), coastal, farm, mine, good_manager
			);
			return ret;
		}
	)(root);

	production_types.lock();

	return ret;
}