#include "ProductionType.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Job::Job(
	PopType const* const new_pop_type, effect_t new_effect_type, fixed_point_t new_effect_multiplier,
	fixed_point_t new_desired_workforce_share
)
	: pop_type { new_pop_type }, effect_type { new_effect_type }, effect_multiplier { new_effect_multiplier },
	  desired_workforce_share { new_desired_workforce_share } {}

ProductionType::ProductionType(
	std::string_view new_identifier, Job new_owner, std::vector<Job> new_jobs, template_type_t new_template_type,
	Pop::pop_size_t new_base_workforce_size, Good::good_map_t&& new_input_goods, Good const* const new_output_goods,
	fixed_point_t new_base_output_quantity, std::vector<bonus_t>&& new_bonuses, Good::good_map_t&& new_maintenance_requirements,
	bool new_is_coastal, bool new_is_farm, bool new_is_mine
)
	: HasIdentifier { new_identifier }, owner { new_owner }, jobs { new_jobs }, template_type { new_template_type },
	  base_workforce_size { new_base_workforce_size }, input_goods { std::move(new_input_goods) },
	  output_goods { new_output_goods }, base_output_quantity { new_base_output_quantity }, bonuses { std::move(new_bonuses) },
	  maintenance_requirements { std::move(new_maintenance_requirements) }, coastal { new_is_coastal }, farm { new_is_farm },
	  mine { new_is_mine } {}

bool ProductionType::parse_scripts(GameManager const& game_manager) {
	bool ret = true;
	for (auto& [bonus_script, bonus_value] : bonuses) {
		ret &= bonus_script.parse_script(false, game_manager);
	}
	return ret;
}

ProductionTypeManager::ProductionTypeManager() : rgo_owner_sprite { 0 } {}

node_callback_t ProductionTypeManager::_expect_job(
	GoodManager const& good_manager, PopManager const& pop_manager, callback_t<Job&&> cb
) {

	return [this, &good_manager, &pop_manager, cb](ast::NodeCPtr node) -> bool {
		using enum Job::effect_t;

		std::string_view pop_type {};
		Job::effect_t effect_type {THROUGHPUT};
		fixed_point_t effect_multiplier = 1, desired_workforce_share = 1;

		static const string_map_t<Job::effect_t> effect_map = {
			{ "input", INPUT }, { "output", OUTPUT }, { "throughput", THROUGHPUT }
		};

		bool res = expect_dictionary_keys(
			"poptype", ONE_EXACTLY, expect_identifier(assign_variable_callback(pop_type)),
			"effect", ONE_EXACTLY, expect_identifier(expect_mapped_string(effect_map, assign_variable_callback(effect_type))),
			"effect_multiplier", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(effect_multiplier)),
			"amount", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(desired_workforce_share))
		)(node);

		PopType const* const found_pop_type = pop_manager.get_pop_type_by_identifier(pop_type);
		return res & cb({ found_pop_type, effect_type, effect_multiplier, desired_workforce_share });
	};
}

node_callback_t ProductionTypeManager::_expect_job_list(
	GoodManager const& good_manager, PopManager const& pop_manager, callback_t<std::vector<Job>&&> cb
) {
	return [this, &good_manager, &pop_manager, cb](ast::NodeCPtr node) -> bool {
		std::vector<Job> jobs;
		bool ret = expect_list(_expect_job(good_manager, pop_manager, vector_callback(jobs)))(node);
		ret &= cb(std::move(jobs));
		return ret;
	};
}

bool ProductionTypeManager::add_production_type(
	std::string_view identifier, Job owner, std::vector<Job> jobs, ProductionType::template_type_t template_type,
	Pop::pop_size_t base_workforce_size, Good::good_map_t&& input_goods, Good const* const output_goods,
	fixed_point_t base_output_quantity, std::vector<ProductionType::bonus_t>&& bonuses,
	Good::good_map_t&& maintenance_requirements, bool is_coastal, bool is_farm, bool is_mine
) {
	if (identifier.empty()) {
		Logger::error("Invalid production type identifier - empty!");
		return false;
	}

	if (base_workforce_size <= 0) {
		Logger::error("Base workforce size ('workforce') for production type ", identifier, " was 0 or unset!");
		return false;
	}

	if (base_output_quantity <= 0) {
		Logger::error("Base output quantity ('value') for production type ", identifier, " was 0 or unset!");
		return false;
	}

	if (output_goods == nullptr) {
		Logger::error("Output good for production type ", identifier, " was null!");
		return false;
	}

	if (template_type == ProductionType::template_type_t::ARTISAN) {
		if (owner.get_pop_type() != nullptr || !jobs.empty()) {
			Logger::warning("Artisanal production types don't use owner and employees. Effects are ignored.");
		}
	} else {
		if (owner.get_pop_type() == nullptr) {
			Logger::error("Production type ", identifier, " lacks owner or has an invalid pop type.");
			return false;
		}

		if (jobs.empty()) {
			Logger::error("Production type ", identifier, " lacks jobs ('employees').");
			return false;
		}

		for (size_t i = 0; i < jobs.size(); i++) {
			if (jobs[i].get_pop_type() == nullptr) {
				Logger::error("Production type ", identifier, " has invalid pop type in employees[", i, "].");
				return false;
			}
		}
	}

	const bool ret = production_types.add_item({
		identifier, owner, jobs, template_type, base_workforce_size, std::move(input_goods),
		output_goods, base_output_quantity, std::move(bonuses), std::move(maintenance_requirements), is_coastal, is_farm, is_mine
	});
	if (rgo_owner_sprite <= 0 && ret && template_type == ProductionType::template_type_t::RGO && owner.get_pop_type() != nullptr) {
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
			using enum ProductionType::template_type_t;

			if (template_node_map.contains(key)) {
				return true;
			}

			Job owner {};
			std::vector<Job> jobs;
			ProductionType::template_type_t template_type { FACTORY };
			Good const* output_goods = nullptr;
			Pop::pop_size_t base_workforce_size = 0; // 0 is a meaningless value -> unset
			Good::good_map_t input_goods, maintenance_requirements;
			fixed_point_t base_output_quantity = 0; // 0 is a meaningless value -> unset
			std::vector<ProductionType::bonus_t> bonuses;
			bool is_coastal = false, is_farm = false, is_mine = false;

			bool ret = true;

			static const string_map_t<ProductionType::template_type_t> template_type_map = {
				{ "factory", FACTORY }, { "rgo", RGO }, { "artisan", ARTISAN }
			};

			const node_callback_t parse_node = expect_dictionary_keys(
				"template", ZERO_OR_ONE, success_callback,
				"bonus", ZERO_OR_MORE, [&bonuses](ast::NodeCPtr bonus_node) -> bool {
					ConditionScript trigger { scope_t::STATE, scope_t::NO_SCOPE, scope_t::NO_SCOPE };
					fixed_point_t bonus_value {};
					const bool ret = expect_dictionary_keys(
						"trigger", ONE_EXACTLY, trigger.expect_script(),
						"value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(bonus_value))
					)(bonus_node);
					bonuses.emplace_back(std::move(trigger), bonus_value);
					return ret;
				},
				"owner", ZERO_OR_ONE, _expect_job(good_manager, pop_manager, move_variable_callback(owner)),
				"employees", ZERO_OR_ONE, _expect_job_list(good_manager, pop_manager, move_variable_callback(jobs)),
				"type", ZERO_OR_ONE, expect_identifier(expect_mapped_string(template_type_map, assign_variable_callback(template_type))),
				"workforce", ZERO_OR_ONE, expect_uint(assign_variable_callback(base_workforce_size)),
				"input_goods", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(input_goods)),
				"output_goods", ZERO_OR_ONE, good_manager.expect_good_identifier(assign_variable_callback_pointer(output_goods)),
				"value", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(base_output_quantity)),
				"efficiency", ZERO_OR_ONE, good_manager.expect_good_decimal_map(move_variable_callback(maintenance_requirements)),
				"is_coastal", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_coastal)),
				"farm", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_farm)),
				"mine", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_mine))
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
				key, owner, jobs, template_type, base_workforce_size, std::move(input_goods), output_goods, base_output_quantity, std::move(bonuses),
				std::move(maintenance_requirements), is_coastal, is_farm, is_mine
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
