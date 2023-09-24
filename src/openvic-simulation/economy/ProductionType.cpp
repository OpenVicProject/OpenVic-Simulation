#include "ProductionType.hpp"
#include <string_view>
#include "dataloader/NodeTools.hpp"
#include "openvic-dataloader/v2script/AbstractSyntaxTree.hpp"
#include "pop/Pop.hpp"
#include "utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

EmployedPop::EmployedPop(PopType const* pop_type, bool artisan, effect_t effect, fixed_point_t effect_multiplier, fixed_point_t amount)
	: pop_type { pop_type }, artisan { artisan }, effect { effect }, effect_multiplier { effect_multiplier }, amount { amount } {}

PopType const* EmployedPop::get_pop_type() {
	return pop_type;
}

bool EmployedPop::is_artisan() {
	return artisan;
}

EmployedPop::effect_t EmployedPop::get_effect() {
	return effect;
}

fixed_point_t EmployedPop::get_effect_multiplier() {
	return effect_multiplier;
}

fixed_point_t EmployedPop::get_amount() {
	return amount;
}

ProductionType::ProductionType(ARGS(type_t, const Good*)) : HasIdentifier { identifier }, owner { owner },
	employees { employees }, type { type }, workforce { workforce }, input_goods { input_goods }, output_goods { output_goods },
	value { value }, bonuses { bonuses }, efficiency { efficiency }, coastal { coastal }, farm { farm }, mine { mine } {}

EmployedPop const& ProductionType::get_owner() const {
	return owner;
}

std::vector<EmployedPop> const& ProductionType::get_employees() const {
	return employees;
}

ProductionType::type_t ProductionType::get_type() const {
	return type;
}

size_t ProductionType::get_workforce() const {
	return workforce;
}

std::map<const Good*, fixed_point_t> const& ProductionType::get_input_goods() {
	return input_goods;
}

const Good* ProductionType::get_output_goods() const {
	return output_goods;
}

fixed_point_t ProductionType::get_value() const {
	return value;
}

std::vector<Bonus> const& ProductionType::get_bonuses() {
	return bonuses;
}

std::map<const Good*, fixed_point_t> const& ProductionType::get_efficiency() {
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

node_callback_t ProductionTypeManager::_expect_employed_pop(GoodManager& good_manager, PopManager& pop_manager,
	callback_t<EmployedPop> cb) {
	
	return [this, &good_manager, &pop_manager, cb](ast::NodeCPtr node) -> bool {
		std::string_view pop_type, effect;
		fixed_point_t effect_multiplier = 1, amount = 1;

		bool res = expect_dictionary_keys(
			"poptype", ONE_EXACTLY, expect_identifier(assign_variable_callback(pop_type)),
			"effect", ONE_EXACTLY, expect_identifier(assign_variable_callback(effect)),
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

		EmployedPop::effect_t found_effect;
		if (effect == "input") found_effect = EmployedPop::effect_t::INPUT;
		else if (effect == "output") found_effect = EmployedPop::effect_t::OUTPUT;
		else if (effect == "throughput") found_effect = EmployedPop::effect_t::THROUGHPUT;
		else {
			Logger::error("Found invalid effect ", effect, " while parsing production types!");
			return false;
		}

		return res & cb(EmployedPop { found_pop_type, artisan, found_effect, effect_multiplier, amount });
	};
}

node_callback_t ProductionTypeManager::_expect_employed_pop_list(GoodManager& good_manager, PopManager& pop_manager,
	callback_t<std::vector<EmployedPop>> cb) {
	
	return [this, &good_manager, &pop_manager, cb](ast::NodeCPtr node) -> bool {
		std::vector<EmployedPop> employed_pops;
		bool res = expect_list([this, &good_manager, &pop_manager, &employed_pops](ast::NodeCPtr node) -> bool {
			EmployedPop owner;
			bool res_partial = _expect_employed_pop(good_manager, pop_manager, assign_variable_callback(owner))(node);
			employed_pops.push_back(owner);
			return res_partial;
		})(node);
		return res & cb(employed_pops);
	};
}

#define POPTYPE_CHECK(employed_pop) \
	if ((employed_pop.pop_type == nullptr && !employed_pop.artisan) || (employed_pop.pop_type != nullptr && employed_pop.artisan)) {\
		Logger::error("Invalid pop type parsed for owner of production type ", identifier, "!"); \
		return false; \
	}

bool ProductionTypeManager::add_production_type(ARGS(std::string_view, std::string_view), GoodManager& good_manager) {
	if (identifier.empty()) {
		Logger::error("Invalid production type identifier - empty!");
		return false;
	}

	ProductionType::type_t type_enum;
	if (type == "factory") type_enum = ProductionType::type_t::FACTORY;
	else if (type == "rgo") type_enum = ProductionType::type_t::RGO;
	else if (type == "artisan") type_enum = ProductionType::type_t::ARTISAN;
	else {
		Logger::error("Bad type ", type, " for production type ", identifier, "!");
		return false;
	}

	if (workforce == 0) {
		Logger::error("Workforce for production type ", identifier, " was 0 or unset!");
		return false;
	}

	if (value == 0) {
		Logger::error("Value for production type ", identifier, " was 0 or unset!");
		return false;
	}

	POPTYPE_CHECK(owner)

	for (int i = 0; i < employees.size(); i++) {
		POPTYPE_CHECK(employees[i])
	}

	const Good* output = good_manager.get_good_by_identifier(output_goods);
	if (output == nullptr) {
		Logger::error("Invalid output ", output_goods, " for production type ", identifier, "!");
		return false;
	}

	return production_types.add_item({ 
		identifier, owner, employees, type_enum, workforce, input_goods,
		output, value, bonuses, efficiency, coastal, farm, mine 
	});
}

#define PARSE_NODE(target_node) expect_dictionary_keys(ALLOW_OTHER_KEYS, \
			"owner", ZERO_OR_ONE, _expect_employed_pop(good_manager, pop_manager, move_variable_callback(owner)), \
			"employees", ZERO_OR_ONE, _expect_employed_pop_list(good_manager, pop_manager, move_variable_callback(employees)), \
			"type", ZERO_OR_ONE, expect_identifier(assign_variable_callback(type)), \
			"workforce", ZERO_OR_ONE, expect_uint(assign_variable_callback(workforce)), \
			"input_goods", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(input_goods)), \
			"output_goods", ZERO_OR_ONE, expect_identifier(assign_variable_callback(output_goods)), \
			"value", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(value)), \
			"efficiency", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(efficiency)), \
			"is_coastal", ZERO_OR_ONE, expect_bool(assign_variable_callback(coastal)), \
			"farm", ZERO_OR_ONE, expect_bool(assign_variable_callback(farm)), \
			"mine", ZERO_OR_ONE, expect_bool(assign_variable_callback(mine)) \
		)(target_node)

bool ProductionTypeManager::load_production_types_file(GoodManager& good_manager, PopManager& pop_manager, ast::NodeCPtr root) {
	size_t expected_types = 0;

	//pass 1: find and store template identifiers
	std::set<std::string_view> templates;
	std::map<std::string_view, std::string_view> template_target_map;
	bool ret = expect_dictionary(
		[this, &expected_types, &templates, &template_target_map](std::string_view key, ast::NodeCPtr value) -> bool {
			std::string_view template_id = "";
			bool ret = expect_dictionary_keys(ALLOW_OTHER_KEYS,
				"template", ZERO_OR_ONE, expect_identifier(assign_variable_callback(template_id))
			)(value);

			if (!template_id.empty()) {
				templates.emplace(template_id);
				template_target_map.emplace(key, template_id);
			}

			expected_types++;

			return ret;
		}
	)(root);

	//pass 2: create and populate the template map
	std::map<std::string_view, ast::NodeCPtr> template_node_map; 
	expect_dictionary(
		[this, &expected_types, &templates, &template_node_map](std::string_view key, ast::NodeCPtr value) -> bool {
			if (templates.contains(key)) {
				template_node_map.emplace(key, value);
				expected_types--;
			}
			return true;
		}
	)(root);

	//pass 3: actually load production types
	production_types.reserve(production_types.size() + expected_types);
	ret &= expect_dictionary(
		[this, &good_manager, &pop_manager, &template_target_map, &template_node_map](std::string_view key, ast::NodeCPtr node) -> bool {
			if (template_node_map.contains(key))
				return true;

			EmployedPop owner;
			std::vector<EmployedPop> employees;
			std::string_view type, output_goods;
			size_t workforce = 0; //0 is a meaningless value -> unset
			std::map<const Good*, fixed_point_t> input_goods, efficiency;
			fixed_point_t value = 0; //0 is a meaningless value -> unset
			std::vector<Bonus> bonuses;
			bool coastal = false, farm = false, mine = false;

			bool ret = true;

			//apply template first
			if (template_target_map.contains(key)) {
				std::string_view template_id = template_target_map[key];
				if (template_node_map.contains(template_id)) {
					ast::NodeCPtr template_node = template_node_map[template_id];
					ret &= PARSE_NODE(template_node);
				}
			}

			ret &= PARSE_NODE(node);
			ret &= add_production_type(
				key, owner, employees, type, workforce, input_goods, output_goods, value, 
				bonuses, efficiency, coastal, farm, mine, good_manager
			);
			return ret;
		}
	)(root);

	production_types.lock();

	return ret;
}