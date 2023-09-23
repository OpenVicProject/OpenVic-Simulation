#include "ProductionType.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

EmployedPop::EmployedPop(const PopType* pop_type, effect_t effect, fixed_point_t effect_multiplier, fixed_point_t amount)
	: pop_type { pop_type }, effect { effect }, effect_multiplier { effect_multiplier }, amount { amount } {}

const PopType* EmployedPop::get_pop_type() const {
	return pop_type;
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

uint32_t ProductionType::get_workforce() const {
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
	
	return [this, &good_manager, &pop_manager, &cb](ast::NodeCPtr node) -> bool {
		std::string_view pop_type, effect;
		fixed_point_t effect_multiplier = 1, amount = 1;

		bool res = expect_dictionary_keys(
			"pop_type", ONE_EXACTLY, expect_identifier(assign_variable_callback(pop_type)),
			"effect", ONE_EXACTLY, expect_identifier(assign_variable_callback(effect)),
			"effect_multiplier", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(effect_multiplier)),
			"amount", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(amount))
		)(node);

		const PopType* found_pop_type = pop_manager.get_pop_type_by_identifier(pop_type);
		if (found_pop_type == nullptr) {
			Logger::error("Found invalid pop type ", pop_type, " while parsing production types!");
			return false;
		}

		EmployedPop::effect_t found_effect;
		if (effect == "input") found_effect = EmployedPop::effect_t::INPUT;
		else if (effect == "output") found_effect = EmployedPop::effect_t::OUTPUT;
		else if (effect == "throughput") found_effect = EmployedPop::effect_t::THROUGHPUT;
		else {
			Logger::error("Found invalid effect ", effect, " while parsing production types!");
			return false;
		}

		return res & cb(EmployedPop { found_pop_type, found_effect, effect_multiplier, amount });
	};
}

node_callback_t ProductionTypeManager::_expect_employed_pop_list(GoodManager& good_manager, PopManager& pop_manager,
	callback_t<std::vector<EmployedPop>> cb) {
	
	return [this, &good_manager, &pop_manager, &cb](ast::NodeCPtr node) -> bool {
		std::vector<EmployedPop> employed_pops;
		bool res = expect_list([this, &good_manager, &pop_manager, &employed_pops](ast::NodeCPtr node) -> bool {
			EmployedPop* owner = nullptr;
			bool res_partial = _expect_employed_pop(good_manager, pop_manager, assign_variable_callback(*owner))(node);
			if (owner != nullptr)
				employed_pops.push_back(*owner);
			return res_partial;
		})(node);
		return res & cb(employed_pops);
	};
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

	const Good* output = good_manager.get_good_by_identifier(output_goods);
	if (output == nullptr) {
		Logger::error("Invalid output for production type ", output_goods, "!");
		return false;
	}

	return production_types.add_item({ 
		identifier, owner, employees, type_enum, workforce, input_goods,
		output, value, bonuses, efficiency, coastal, farm, mine 
	});
}

bool ProductionTypeManager::load_production_types_file(GoodManager& good_manager, PopManager& pop_manager, ast::NodeCPtr root) {
	size_t expected_types = 0;

	//pass 1: find and store template identifiers
	std::set<std::string_view> templates;
	bool ret = expect_dictionary([this, &expected_types, &templates](std::string_view key, ast::NodeCPtr value) -> bool {
		std::string_view template_id = "";
		bool ret = expect_dictionary_keys(ALLOW_OTHER_KEYS,
			"template", ZERO_OR_ONE, expect_identifier(assign_variable_callback(template_id))
		)(value);

		if (!template_id.empty())
			templates.emplace(template_id);
		else expected_types++;

		return ret;
	})(root);

	//pass 2: create and populate the template map
	std::map<std::string_view, ast::NodeCPtr> template_map; 
	expect_dictionary([this, &templates, &template_map](std::string_view key, ast::NodeCPtr value) -> bool {
		if (templates.contains(key))
			template_map.emplace(key, value);
		return true;
	})(root);

	//pass 3: actually load production types
	production_types.reserve(production_types.size() + expected_types);
	ret &= expect_dictionary(
		[this, &good_manager, &pop_manager, &template_map](std::string_view key, ast::NodeCPtr node) -> bool {
			EmployedPop* owner;
			std::vector<EmployedPop> employees;
			std::string_view type;
			uint32_t workforce;
			std::map<const Good*, fixed_point_t> input_goods;
			std::string_view output_goods;
			fixed_point_t value;
			std::vector<Bonus> bonuses;
			std::map<const Good*, fixed_point_t> efficiency;
			bool coastal = false; //is_coastal
			bool farm = false;
			bool mine = false;

			return expect_dictionary_keys(ALLOW_OTHER_KEYS,
				"owner", ONE_EXACTLY, _expect_employed_pop(
					good_manager, pop_manager, move_variable_callback(*owner)),
				"employees", ONE_EXACTLY, _expect_employed_pop_list(
					good_manager, pop_manager, move_variable_callback(employees)),
				"type", ONE_EXACTLY, expect_identifier(assign_variable_callback(type)),
				"workforce", ONE_EXACTLY, expect_uint(assign_variable_callback(workforce)),
				"input_goods", ZERO_OR_ONE, good_manager.expect_good_decimal_map(assign_variable_callback(input_goods)),
				"output_goods_id", ONE_EXACTLY, expect_identifier(assign_variable_callback(output_goods)),
				"value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(value)),
				//"bonus", ONE_OR_MORE, TODO
				"efficiency", ZERO_OR_ONE, good_manager.expect_good_decimal_map(assign_variable_callback(efficiency)),
				"is_coastal", ZERO_OR_ONE, expect_bool(assign_variable_callback(coastal)),
				"farm", ZERO_OR_ONE, expect_bool(assign_variable_callback(farm)),
				"mine", ZERO_OR_ONE, expect_bool(assign_variable_callback(mine))
			)(node) & add_production_type(
				key, *owner, employees, type, workforce, input_goods, output_goods, value, 
				bonuses, efficiency, coastal, farm, mine, good_manager
			);
		}
	)(root);

	production_types.lock();

	return ret;
}