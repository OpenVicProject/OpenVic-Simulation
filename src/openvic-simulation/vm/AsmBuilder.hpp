#pragma once

#include <lauf/asm/builder.h>
#include <lauf/asm/module.h>

namespace OpenVic::Vm {
	struct AsmBuilder {
		AsmBuilder(lauf_asm_build_options options) : _handle(lauf_asm_create_builder(options)) {}

		AsmBuilder(AsmBuilder&&) = default;
		AsmBuilder& operator=(AsmBuilder&&) = default;

		AsmBuilder(AsmBuilder const&) = delete;
		AsmBuilder& operator=(AsmBuilder const&) = delete;

		~AsmBuilder() {
			lauf_asm_destroy_builder(_handle);
		}

		lauf_asm_builder* handle() {
			return _handle;
		}

		const lauf_asm_builder* handle() const {
			return _handle;
		}

		operator lauf_asm_builder*() {
			return _handle;
		}

		operator const lauf_asm_builder*() const {
			return _handle;
		}

		struct CodeBuilder {
			CodeBuilder(CodeBuilder const&) = delete;
			CodeBuilder& operator=(CodeBuilder const&) = delete;

			~CodeBuilder() {
				if (!has_finished()) {
					finish();
				}
			}

			operator lauf_asm_builder*() {
				return _builder.handle();
			}

			operator const lauf_asm_builder*() const {
				return _builder.handle();
			}

			bool finish() {
				return _has_finished = lauf_asm_build_finish(_builder.handle());
			}

			bool has_finished() const {
				return _has_finished;
			}

			bool is_well_formed() const {
				return _is_well_formed;
			}

			void build_function(lauf_asm_function* fn) {
				lauf_asm_build(_builder.handle(), _module, fn);
			}

			void build_chunk(lauf_asm_chunk* chunk, lauf_asm_signature sig) {
				lauf_asm_build_chunk(_builder.handle(), _module, chunk, sig);
			}

		private:
			friend struct AsmBuilder;
			CodeBuilder(AsmBuilder& builder, lauf_asm_module* module) : _builder(builder), _module(module) {}

			bool _has_finished;
			bool _is_well_formed;
			AsmBuilder& _builder;
			lauf_asm_module* _module;
		};

		CodeBuilder code(lauf_asm_module* module) {
			return CodeBuilder(*this, module);
		}

	private:
		lauf_asm_builder* _handle;
	};
}
