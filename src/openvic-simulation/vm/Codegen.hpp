#pragma once

#include <vector>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include "AsmBuilder.hpp"
#include "GameManager.hpp"
#include "Module.hpp"
#include "types/EnumBitfield.hpp"
#include "vm/Builtin.hpp"
#include "vm/VirtualMachine.hpp"
#include <lauf/asm/builder.h>
#include <lauf/vm.h>

namespace OpenVic::Vm {
	struct Codegen {
		Codegen(
			ovdl::v2script::Parser const& parser, GameManager* instance_manager, const char* module_name,
			lauf_asm_build_options options = lauf_asm_default_build_options
		);

		Codegen(ovdl::v2script::Parser const& parser, GameManager* instance_manager, Module&& module, AsmBuilder&& builder);

		operator lauf_asm_module*() {
			return _module;
		}

		operator const lauf_asm_module*() const {
			return _module;
		}

		operator lauf_asm_builder*() {
			return _builder;
		}

		operator const lauf_asm_builder*() const {
			return _builder;
		}

		Module& module() {
			return _module;
		}

		Module const& module() const {
			return _module;
		}

		AsmBuilder& builder() {
			return _builder;
		}

		AsmBuilder const& builder() const {
			return _builder;
		}

		OpenVicVirtualMachine create_virtual_machine() && {
			return { _game_manager, std::move(_scope_definitions) };
		}

		lauf_asm_block* create_block(size_t input_count) {
			return lauf_asm_declare_block(_builder, input_count);
		}

		void set_working_block(lauf_asm_block* block) {
			lauf_asm_build_block(*this, block);
		}

		enum class scope_execution_type : std::uint8_t { Effect, Trigger };
		enum class scope_type : std::uint8_t {
			None = 0,
			Country = 1 << 0,
			State = 1 << 1,
			Province = 1 << 2,
			Pop = 1 << 3,

			Generic = Country | State | Province | Pop,
		};

		enum class trigger_modifier : std::uint8_t { //
			Not = 1 << 0,
			And = 1 << 1,
			Or = 1 << 2,
		};

		bool
		is_iterative_scope(scope_execution_type execution_type, scope_type active_scope_type, ovdl::symbol<char> name) const;
		scope_type get_scope_type_for(scope_execution_type execution_type, ovdl::symbol<char> name) const;

		OpenVic::Asm::scope_store_variant get_static_scope( //
			scope_execution_type execution_type, scope_type active_scope_type, ovdl::symbol<char> name
		) const;

		bool supports_limit(scope_execution_type execution_type, scope_type active_scope_type, ovdl::symbol<char> name) const;

		lauf_asm_function* create_effect_function(scope_type type, const ovdl::v2script::ast::Node* node);
		lauf_asm_function* create_condition_function(scope_type type, const ovdl::v2script::ast::Node* node);

		void generate_effect_from(scope_type type, const ovdl::v2script::ast::Node* node);
		void generate_condition_from(scope_type type, const ovdl::v2script::ast::Node* node);

		bool inst_store_ov_asm_key_null(lauf_asm_local* local, std::size_t index);
		bool inst_store_ov_asm_key(lauf_asm_local* local, std::size_t index, ovdl::symbol<char> key);
		bool inst_store_ov_asm_value_from_vstack(lauf_asm_local* local, std::size_t index, std::uint8_t type);
		bool inst_store_ov_asm_type(lauf_asm_local* local, std::size_t index, std::uint8_t type);

		bool push_instruction_for_scope(scope_execution_type execution_type, scope_type type, ovdl::symbol<char> scope_symbol);

		// Bytecode instructions //
		void inst_push_scope_this();
		void inst_push_scope_from();

		void inst_push_get_country_capital();
		void inst_push_get_country_cultural_union();
		void inst_push_get_country_overlord();
		void inst_push_get_country_sphere_owner();

		void inst_push_get_province_controller();
		void inst_push_get_province_owner();
		void inst_push_get_province_state();

		void inst_push_get_state_cultural_union();

		void inst_push_get_pop_location();
		void inst_push_get_pop_country();
		void inst_push_get_pop_cultural_union();

		void inst_push_get_random_country();
		void inst_push_get_random_owned();

		void inst_push_get_random_neighbor_province();
		void inst_push_get_random_empty_neighbor_province();

		void inst_push_get_this_union();

		void inst_push_get_tag(const char* nation_id);
		void inst_push_get_province(const char* province_id);
		void inst_push_get_region(const char* region_id);
		void inst_push_get_continent(const char* continent_id);

		void inst_push_get_pop_type(const char* pop_type_id);
		void inst_push_get_unit_type(const char* unit_type_id);
		void inst_push_get_factory_type(const char* factory_type_id);
		void inst_push_get_building_type(const char* building_type_id);
		void inst_push_get_crime_type(const char* crime_type_id);

		void inst_push_get_culture(const char* culture_id);
		void inst_push_get_religion(const char* religion_id);

		void inst_push_get_economic_reform(const char* economic_reform_id);
		void inst_push_get_social_reform(const char* social_reform_id);
		void inst_push_get_political_reform(const char* political_reform_id);

		void inst_push_get_trade_policy(const char* trade_policy_id);
		void inst_push_get_war_policy(const char* war_policy_id);
		void inst_push_get_citizenship_policy(const char* citizenship_policy_id);

		void inst_push_get_issue(const char* issue_id);
		void inst_push_get_ideology(const char* ideology_id);
		void inst_push_get_party(const char* party_id);

		void inst_push_get_military_leader(const char* military_leader_id);
		void inst_push_get_personality_trait(const char* personality_trait_id);
		void inst_push_get_background_trait(const char* background_trait_id);

		void inst_push_get_event(const char* event_id);

		void inst_push_get_trade_good(const char* trade_good_id);
		void inst_push_get_resource(const char* resource_id);

		void inst_push_get_technology(const char* technology_id);
		void inst_push_get_government(const char* government_id);
		void inst_push_get_casus_belli(const char* casus_belli_id);
		void inst_push_get_national_value(const char* national_value_id);

		void inst_push_get_variable(const char* variable_id);

		void inst_push_get_global_flag(const char* flag_id);

		void inst_push_get_country_flag(const char* flag_id);
		void inst_push_get_country_modifier(const char* modifier_id);

		void inst_push_get_province_flag(const char* flag_id);
		void inst_push_get_province_modifier(const char* modifier_id);
		// Bytecode instructions //

		struct symbol_hash {
			std::size_t operator()(const ovdl::symbol<char>& s) const noexcept {
				return std::hash<const void*> {}(static_cast<const void*>(s.c_str()));
			}
		};

	private:
		std::vector<OpenVic::Asm::scope_store_variant> _scope_definitions;
		GameManager* _game_manager;
		Module _module;
		AsmBuilder _builder;
		ovdl::v2script::Parser const& _parser;
		bool _has_top_level_country_scopes;
		bool _has_top_level_province_scopes;
	};
}

template<>
struct OpenVic::enable_bitfield<OpenVic::Vm::Codegen::scope_type> : std::true_type {};

template<>
struct OpenVic::enable_bitfield<OpenVic::Vm::Codegen::trigger_modifier> : std::true_type {};
