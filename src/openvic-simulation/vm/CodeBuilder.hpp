#pragma once

#include <bit>
#include <string_view>

#include "openvic-simulation/vm/Handler.hpp"
#include "openvic-simulation/vm/Local.hpp"
#include "openvic-simulation/vm/Module.hpp"

#include <lauf/asm/builder.h>
#include <lauf/asm/module.h>

namespace OpenVic::Vm {
	struct AsmBuilderRef;

	struct CodeBuilder : HandlerRef<lauf_asm_builder> {
		AsmBuilderRef asm_builder();

		ModuleRef module() {
			return _module;
		}

		bool is_function() const {
			return _body.function.is_function;
		}

		bool is_chunk() const {
			return !_body.chunk.is_function && _body.chunk.value != nullptr;
		}

		bool can_build() const {
			return !_body.chunk.is_function && _body.chunk.value == nullptr;
		}

		Function function() const {
			assert(is_function());
			return _body.function.value;
		}

		Chunk chunk() const {
			assert(is_chunk());
			return _body.chunk.value;
		}

		Global build_literal(const std::span<unsigned char> data) {
			return lauf_asm_build_data_literal(*this, data.data(), data.size_bytes());
		}

		Global build_string(std::string_view string) {
			return lauf_asm_build_data_literal(
				*this, std::bit_cast<unsigned char*>(string.data()), string.size() * sizeof(char)
			);
		}

		Global build_cstring(const char* string) {
			return lauf_asm_build_string_literal(*this, string);
		}

		Local build_local(lauf_asm_layout layout) {
			return lauf_asm_build_local(*this, layout);
		}

		void drop() && {
			_body.chunk.is_function = false;
			_body.chunk.value = nullptr;
		}

		bool finish() && {
			bool result = lauf_asm_build_finish(*this);
			static_cast<CodeBuilder&&>(*this).drop();
			return result;
		}

		~CodeBuilder() {
			if (_handle == nullptr || !can_build()) {
				return;
			}
			static_cast<CodeBuilder&&>(*this).finish();
			_handle = nullptr;
		}

	protected:
		friend struct AsmBuilderRef;
		CodeBuilder(AsmBuilderRef builder);
		CodeBuilder(AsmBuilderRef builder, ModuleRef module, Function function);
		CodeBuilder(AsmBuilderRef builder, ModuleRef module, Chunk chunk, lauf_asm_signature signature);

		ModuleRef _module;

		union {
			struct {
				bool is_function;
				lauf_asm_function* value;
			} function;
			struct {
				bool is_function;
				lauf_asm_chunk* value;
			} chunk;
		} _body;
	};
}
