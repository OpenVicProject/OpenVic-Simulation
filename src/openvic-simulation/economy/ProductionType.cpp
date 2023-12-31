#include "ProductionType.hpp"

#include "openvic-simulation/types/OrderedContainers.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

EmployedPop::EmployedPop(
	PopType const* new_pop_type, bool new_artisan, effect_t new_effect, fixed_point_t new_effect_multiplier,
	fixed_point_t new_amount
) : pop_type { new_pop_type }, artisan { new_artisan }, effect { new_effect }, effect_multiplier { new_effect_multiplier },
	amount { new_amount } {}

ProductionType::ProductionType(
	std::string_view new_identifier, EmployedPop new_owner, std::vector<EmployedPop> new_employees, type_t new_type,
	Pop::pop_size_t new_workforce, Good::good_map_t&& new_input_goods, Good const* new_output_goods,
	fixed_point_t new_value, std::vector<bonus_t>&& new_bonuses, Good::good_map_t&& new_efficiency, bool new_coastal,
	bool new_farm, bool new_mine
) : HasIdentifier { new_identifier }, owner { new_owner }, employees { new_employees }, type { new_type },
	workforce { new_workforce }, input_goods { std::move(new_input_goods) }, output_goods { new_output_goods },
	value { new_value }, bonuses { std::move(new_bonuses) }, efficiency { std::move(new_efficiency) },
	coastal { new_coastal }, farm { new_farm }, mine { new_mine } {}

bool ProductionType::parse_scripts(GameManager const& game_manager) {
	bool ret = true;
	for (auto& [bonus_script, bonus_value] : bonuses) {
		ret &= bonus_script.parse_script(false, game_manager);
	}
	return ret;
}

ProductionTypeManager::ProductionTypeManager() : rgo_owner_sprite { 0 } {}

node_callback_t ProductionTypeManager::_expect_employed_pop(
	GoodManager const& good_manager, PopManager const& pop_manager, callback_t<EmployedPop&&> cb
) {

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

node_callback_t ProductionTypeManager::_expect_employed_pop_list(
	GoodManager const& good_manager, PopManager const& pop_manager, callback_t<std::vector<EmployedPop>&&> cb
) {
	return [this, &good_manager, &pop_manager, cb](ast::NodeCPtr node) -> bool {
		std::vector<EmployedPop> employed_pops;
		bool ret = expect_list(_expect_employed_pop(good_manager, pop_manager, vector_callback(employed_pops)))(node);
		ret &= cb(std::move(employed_pops));
		return ret;
	};
}

#define POPTYPE_CHECK(employed_pop) \
	if ((employed_pop.pop_type == nullptr && !employed_pop.artisan) || \
		(employed_pop.pop_type != nullptr && employed_pop.artisan)) { \
		Logger::error("Invalid pop type parsed for owner of production type ", identifier, "!"); \
		return false; \
	}

bool ProductionTypeManager::add_production_type(
	std::string_view identifier, EmployedPop owner, std::vector<EmployedPop> employees, ProductionType::type_t type,
	Pop::pop_size_t workforce, Good::good_map_t&& input_goods, Good const* output_goods, fixed_point_t value,
	std::vector<ProductionType::bonus_t>&& bonuses, Good::good_map_t&& efficiency, bool coastal, bool farm, bool mine
) {
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

	const bool ret = production_types.add_item({
		identifier, owner, employees, type, workforce, std::move(input_goods),
		output_goods, value, std::move(bonuses), std::move(efficiency), coastal, farm, mine
	});
	if (rgo_owner_sprite <= 0 && ret && type == ProductionType::type_t::RGO && owner.get_pop_type() != nullptr) {
		/* Set rgo owner sprite to that of the first RGO owner we find. */
		rgo_owner_sprite = owner.get_pop_type()->get_sprite();
	}
	return ret;
}

bool ProductionTypeManager::load_production_types_file(
	GoodManager const& good_manager, PopManager const& pop_manager, ast::NodeCPtr root
) {
	size_t expected_types = 0;

	// pass 1: find and store template identifiers
	ordered_set<std::string_view> templates;
	ordered_map<std::string_view, std::string_view> template_target_map;
	bool ret = expect_dictionary(
		[this, &expected_types, &templates, &template_target_map](std::string_view key, ast::NodeCPtr value) -> bool {
			expected_types++;

			std::string_view template_id = "";
			bool found_template = false;
			const bool ret =
				expect_key("template", expect_identifier(assign_variable_callback(template_id)), &found_template)(value);
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
	ordered_map<std::string_view, ast::NodeCPtr> template_node_map;
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
			if (template_node_map.contains(key)) {
				return true;
			}

			EmployedPop owner;
			std::vector<EmployedPop> employees;
			ProductionType::type_t type;
			Good const* output_goods = nullptr;
			Pop::pop_size_t workforce = 0; // 0 is a meaningless value -> unset
			Good::good_map_t input_goods, efficiency;
			fixed_point_t value = 0; // 0 is a meaningless value -> unset
			std::vector<ProductionType::bonus_t> bonuses;
			bool coastal = false, farm = false, mine = false;

			bool ret = true;

			using enum ProductionType::type_t;
			static const string_map_t<ProductionType::type_t> type_map = {
				{ "factory", FACTORY }, { "rgo", RGO }, { "artisan", ARTISAN }
			};

			const node_callback_t parse_node = expect_dictionary_keys(
				"template", ZERO_OR_ONE, success_callback,
				"bonus", ZERO_OR_MORE, [&bonuses](ast::NodeCPtr bonus_node) -> bool {
					ConditionScript trigger;
					fixed_point_t value {};
					const bool ret = expect_dictionary_keys(
						"trigger", ONE_EXACTLY, trigger.expect_script(),
						"value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(value))
					)(bonus_node);
					bonuses.emplace_back(std::move(trigger), value);
					return ret;
				},
				"owner", ZERO_OR_ONE, _expect_employed_pop(good_manager, pop_manager, move_variable_callback(owner)),
				"employees", ZERO_OR_ONE, _expect_employed_pop_list(good_manager, pop_manager, move_variable_callback(employees)),
				"type", ZERO_OR_ONE, expect_identifier(expect_mapped_string(type_map, assign_variable_callback(type))),
				"workforce", ZERO_OR_ONE, expect_uint(assign_variable_callback(workforce)),
				"input_goods", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(input_goods)),
				"output_goods", ZERO_OR_ONE, good_manager.expect_good_identifier(assign_variable_callback_pointer(output_goods)),
				"value", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(value)),
				"efficiency", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(efficiency)),
				"is_coastal", ZERO_OR_ONE, expect_bool(assign_variable_callback(coastal)),
				"farm", ZERO_OR_ONE, expect_bool(assign_variable_callback(farm)),
				"mine", ZERO_OR_ONE, expect_bool(assign_variable_callback(mine))
			);

			// apply template first
			{
				const typename decltype(template_target_map)::const_iterator target_it = template_target_map.find(key);
				if (target_it != template_target_map.end()) {
					const std::string_view template_id = target_it->second;
					const typename decltype(template_node_map)::const_iterator node_it = template_node_map.find(template_id);
					if (node_it != template_node_map.end()) {
						ret &= parse_node(node_it->second);
					} else {
						Logger::error("Missing template ", template_id, " for production type ", key, "!");
						ret = false;
					}
				}
			}

			ret &= parse_node(node);

			ret &= add_production_type(
				key, owner, employees, type, workforce, std::move(input_goods), output_goods, value, std::move(bonuses),
				std::move(efficiency), coastal, farm, mine
			);
			return ret;
		}
	)(root);

	production_types.lock();

	if (rgo_owner_sprite <= 0) {
		Logger::error("No RGO owner pop type sprite found!");
		ret = false;
	}

	return ret;
}

bool ProductionTypeManager::parse_scripts(GameManager const& game_manager) {
	bool ret = true;
	for (ProductionType& production_type : production_types.get_items()) {
		ret &= production_type.parse_scripts(game_manager);
	}
	return ret;
}
