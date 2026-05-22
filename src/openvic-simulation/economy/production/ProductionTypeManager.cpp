#include "ProductionTypeManager.hpp"

#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/core/error/ErrorMacros.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/economy/GoodDefinitionManager.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/population/PopManager.hpp"
#include "openvic-simulation/population/PopSize.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

ProductionType const* ProductionTypeManager::get_good_to_rgo_production_type(GoodDefinition const& key) const {
	return good_to_rgo_production_type.at(key);
}

NodeTools::node_callback_t ProductionTypeManager::_expect_job(
	GoodDefinitionManager const& good_definition_manager,
	PopManager const& pop_manager,
	NodeTools::callback_t<
		pop_type_index_t,
		Job::effect_t,
		fixed_point_t,
		fixed_point_t
	> emplace_callback
) {
	return [this, &good_definition_manager, &pop_manager, emplace_callback](ovdl::v2script::ast::Node const* node) mutable -> bool {
		using enum Job::effect_t;

		std::string_view pop_type_identifier {};
		Job::effect_t effect_type { THROUGHPUT };
		fixed_point_t effect_multiplier = 1, desired_workforce_share = 1;

		static const string_map_t<Job::effect_t> effect_map = {
			{ "input", INPUT }, { "output", OUTPUT }, { "throughput", THROUGHPUT }
		};

		if(!expect_dictionary_keys(
			"poptype", ONE_EXACTLY, expect_identifier(assign_variable_callback(pop_type_identifier)),
			"effect", ONE_EXACTLY, expect_identifier(expect_mapped_string(effect_map, assign_variable_callback(effect_type))),
			"effect_multiplier", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(effect_multiplier)),
			"amount", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(desired_workforce_share))
		)(node)) {
			return false;
		}

		PopType const* const found_pop_type = pop_manager.get_pop_type_by_identifier(pop_type_identifier);
		if (found_pop_type == nullptr) {
			return pop_type_identifier == "artisan" // Victoria 2 misconfiguration
				? true
				: false;
		}
		return emplace_callback(found_pop_type->index, effect_type, effect_multiplier, desired_workforce_share);
	};
}

NodeTools::node_callback_t ProductionTypeManager::_expect_job_list(
	GoodDefinitionManager const& good_definition_manager, PopManager const& pop_manager,
	NodeTools::callback_t<memory::vector<Job>&&> callback
) {
	return [this, &good_definition_manager, &pop_manager, callback](ovdl::v2script::ast::Node const* node) mutable -> bool {
		memory::vector<Job> jobs;
		bool ret = expect_list(
			_expect_job(
				good_definition_manager,
				pop_manager,
				vector_emplace_callback(jobs)
			)
		)(node);
		ret &= callback(std::move(jobs));
		return ret;
	};
}

bool ProductionTypeManager::add_production_type(
	GameRulesManager const& game_rules_manager,
	TypedSpan<pop_type_index_t, const PopType> pop_types,	
	const std::string_view identifier,
	std::optional<Job>&& owner_before_move,
	memory::vector<Job>&& jobs,
	const ProductionType::template_type_t template_type,
	const pop_size_t base_workforce_size,
	fixed_point_map_t<GoodDefinition const*>&& input_goods,
	GoodDefinition const* const output_good,
	const fixed_point_t base_output_quantity,
	memory::vector<ProductionType::bonus_t>&& bonuses,
	fixed_point_map_t<GoodDefinition const*>&& maintenance_requirements,
	const bool is_coastal,
	const bool is_farm,
	const bool is_mine
) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid production type identifier - empty!");
		return false;
	}

	if (base_workforce_size <= 0) {
		spdlog::error_s("Base workforce size ('workforce') for production type {} was 0 or unset!", identifier);
		return false;
	}

	if (base_output_quantity <= 0) {
		spdlog::error_s("Base output quantity ('value') for production type {} was 0 or unset!", identifier);
		return false;
	}

	if (output_good == nullptr) {
		spdlog::error_s("Output good for production type {} was null!", identifier);
		return false;
	}

	using enum ProductionType::template_type_t;

	if (template_type == ARTISAN) {
		if (owner_before_move.has_value()) {
			spdlog::warn_s(
				"Artisanal production type {} should not have an owner - it is being ignored.", identifier
			);
			owner_before_move.reset();
		}

		if (!jobs.empty()) {
			spdlog::warn_s(
				"Artisanal production type {} should not have employees - {} are being ignored.",
				identifier, jobs.size()
			);
			jobs.clear();
		}
	} else {
		if (!owner_before_move.has_value()) {
			spdlog::error_s("Production type {} is missing an owner.", identifier);
			return false;
		}

		if (jobs.empty()) {
			spdlog::error_s("Production type {} lacks jobs ('employees').", identifier);
			return false;
		}
	}

	const bool ret = production_types.emplace_item(
		identifier,
		game_rules_manager, identifier, std::move(owner_before_move), std::move(jobs), template_type, base_workforce_size, std::move(input_goods), *output_good,
		base_output_quantity, std::move(bonuses), std::move(maintenance_requirements), is_coastal, is_farm, is_mine
	);

	if (ret && (template_type == RGO)) {
		ProductionType const& production_type = production_types.back();

		ProductionType const*& current_rgo_pt = good_to_rgo_production_type.at(*output_good);
		if (current_rgo_pt == nullptr || (
			(is_farm && !is_mine)
			&& (!current_rgo_pt->_is_farm || current_rgo_pt->_is_mine)
		)) {
			// pure farms are preferred over alternatives. Use first pt otherwise.
			current_rgo_pt = &get_back_production_type();
		}
		//else ignore, we already have an rgo pt

		std::optional<Job> const& owner_after_move = production_type.owner;
		if (rgo_owner_sprite <= 0
			&& template_type == RGO
			&& owner_after_move.has_value()
		) {
			/* Set rgo owner sprite to that of the first RGO owner we find. */
			rgo_owner_sprite = pop_types[owner_after_move->pop_type_index].sprite;
		}
	}

	return ret;
}

bool ProductionTypeManager::load_production_types_file(
	GameRulesManager const& game_rules_manager,
	GoodDefinitionManager const& good_definition_manager,
	PopManager const& pop_manager,
	ovdl::v2script::Parser const& parser
) {
	using namespace std::string_view_literals;
	auto template_symbol = parser.find_intern("template"sv);
	if (!template_symbol) {
		spdlog::error_s("template could not be interned.");
	}
	auto output_goods_symbol = parser.find_intern("output_goods"sv);
	if (!output_goods_symbol) {
		spdlog::error_s("output_goods could not be interned.");
	}

	size_t expected_types = 0;

	/* Pass #1: find and store template identifiers */
	ordered_set<std::string_view> templates;
	ordered_map<std::string_view, std::string_view> template_target_map;
	bool ret = expect_dictionary(
		[this, &expected_types, &templates, &template_target_map, &template_symbol, &output_goods_symbol]
		(std::string_view key, ovdl::v2script::ast::Node const* value) -> bool {
			expected_types++;

			std::string_view template_id = "";
			bool has_found_template = false;
			const bool is_parsing_template_success = expect_key(
				template_symbol,
				expect_identifier(assign_variable_callback(template_id)),
				&has_found_template
			)(value);

			if (has_found_template) {
				OV_ERR_FAIL_COND_V_MSG(
					!is_parsing_template_success,
					false,
					memory::fmt::format("Failed get template identifier for {}", key)
				);
				templates.emplace(template_id);
				template_target_map.emplace(key, template_id);
			} else {
				bool has_found_output_goods = false;
				expect_key(
					output_goods_symbol, 
					success_callback,
					&has_found_output_goods
				)(value);
				if (!has_found_output_goods) {
					//assume it's a template
					templates.emplace(key);
				}
			}

			return true;
		}
	)(parser.get_file_node());

	/* Pass #2: create and populate the template map */
	ordered_map<std::string_view, ovdl::v2script::ast::Node const*> template_node_map;
	ret &= expect_dictionary(
		[this, &expected_types, &templates, &template_node_map](std::string_view key, ovdl::v2script::ast::Node const* value) -> bool {
			if (templates.contains(key)) {
				template_node_map.emplace(key, value);
				expected_types--;
			}
			return true;
		}
	)(parser.get_file_node());

	/* Pass #3: actually load production types */
	good_to_rgo_production_type = std::move(decltype(good_to_rgo_production_type){good_definition_manager.get_good_definitions()});

	reserve_more_production_types(expected_types);
	ret &= expect_dictionary(
		[this, &game_rules_manager, &good_definition_manager, &pop_manager, &template_target_map, &template_node_map](
			std::string_view key, ovdl::v2script::ast::Node const* node) -> bool {
			using enum ProductionType::template_type_t;

			if (template_node_map.contains(key)) {
				return true;
			}

			std::optional<Job> owner;
			memory::vector<Job> jobs;
			ProductionType::template_type_t template_type { FACTORY };
			GoodDefinition const* output_good = nullptr;
			pop_size_t base_workforce_size = 0;
			fixed_point_map_t<GoodDefinition const*> input_goods, maintenance_requirements;
			fixed_point_t base_output_quantity = 0;
			memory::vector<ProductionType::bonus_t> bonuses;
			bool is_coastal = false, is_farm = false, is_mine = false;

			bool ret = true;

			static const string_map_t<ProductionType::template_type_t> template_type_map = {
				{ "factory", FACTORY }, { "rgo", RGO }, { "artisan", ARTISAN }
			};

			auto parse_node = expect_dictionary_keys(
				"template", ZERO_OR_ONE, success_callback, /* Already parsed using expect_key in Pass #1 above. */
				"bonus", ZERO_OR_MORE, [&bonuses](ovdl::v2script::ast::Node const* bonus_node) -> bool {
					using enum scope_type_t;

					ConditionScript trigger { STATE, NO_SCOPE, NO_SCOPE };
					fixed_point_t bonus_value {};

					const bool ret = expect_dictionary_keys(
						"trigger", ONE_EXACTLY, trigger.expect_script(),
						"value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(bonus_value))
					)(bonus_node);

					bonuses.emplace_back(std::move(trigger), bonus_value);

					return ret;
				},
				"owner", ZERO_OR_ONE, _expect_job(good_definition_manager, pop_manager, emplace_opt_callback(owner)),
				"employees", ZERO_OR_ONE, _expect_job_list(good_definition_manager, pop_manager, move_variable_callback(jobs)),
				"type", ZERO_OR_ONE,
					expect_identifier(expect_mapped_string(template_type_map, assign_variable_callback(template_type))),
				"workforce", ZERO_OR_ONE, expect_strong_typedef<pop_size_t>(assign_variable_callback(base_workforce_size)),
				"input_goods", ZERO_OR_ONE,
					good_definition_manager.expect_good_definition_decimal_map(move_variable_callback(input_goods)),
				"output_goods", ZERO_OR_ONE,
					good_definition_manager.expect_good_definition_identifier(assign_variable_callback_pointer(output_good)),
				"value", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(base_output_quantity)),
				"efficiency", ZERO_OR_ONE, good_definition_manager.expect_good_definition_decimal_map(
					move_variable_callback(maintenance_requirements)
				),
				"is_coastal", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_coastal)),
				"farm", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_farm)),
				"mine", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_mine))
			);

			/* Check if this ProductionType has a template, and if so parse it. */
			{
				const typename decltype(template_target_map)::const_iterator target_it = template_target_map.find(key);
				if (target_it != template_target_map.end()) {
					const std::string_view template_id = target_it->second;
					const typename decltype(template_node_map)::const_iterator node_it = template_node_map.find(template_id);
					if (node_it != template_node_map.end()) {
						ret &= parse_node(node_it->second);
					} else {
						spdlog::error_s(
							"Missing template {} for production type {}!",
							template_id, key
						);
						ret = false;
					}
				}
			}

			/* Parse the ProductionType's own entries, over those of its template if necessary. */
			ret &= parse_node(node);

			ret &= add_production_type(
				game_rules_manager,
				pop_manager.get_pop_types(),
				key, std::move(owner), std::move(jobs), template_type, base_workforce_size, std::move(input_goods), output_good,
				base_output_quantity, std::move(bonuses), std::move(maintenance_requirements), is_coastal, is_farm, is_mine
			);

			return ret;
		}
	)(parser.get_file_node());

	production_types.lock();

	if (rgo_owner_sprite <= 0) {
		spdlog::error_s("No RGO owner pop type sprite found!");
		ret = false;
	}

	return ret;
}

bool ProductionTypeManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	for (ProductionType& production_type : production_types.get_items()) {
		ret &= production_type.parse_scripts(definition_manager);
	}
	return ret;
}
