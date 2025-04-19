#pragma once

#include <bit>
#include <string_view>

#include "openvic-simulation/vm/Chunk.hpp"
#include "openvic-simulation/vm/Function.hpp"
#include "openvic-simulation/vm/Handler.hpp"
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

		void set_for(Function function);
		void set_for(ModuleRef module, Function function);
		void set_for(Chunk chunk);
		void set_for(ModuleRef module, Chunk chunk);
		void set_for(Chunk chunk, lauf_asm_signature signature);
		void set_for(ModuleRef module, Chunk chunk, lauf_asm_signature signature);

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

		bool finish() {
			bool result = lauf_asm_build_finish(*this);
			_body.chunk.is_function = false;
			_body.chunk.value = nullptr;
			return result;
		}

		~CodeBuilder() {
			if (_handle == nullptr || !can_build()) {
				return;
			}
			finish();
			_handle = nullptr;
		}

	protected:
		friend struct AsmBuilderRef;
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

	struct AsmBuilderRef : HandlerRef<lauf_asm_builder> {
		using HandlerRef::HandlerRef;

		CodeBuilder build(ModuleRef module, Function function) {
			return { *this, module, function };
		}

		CodeBuilder build(ModuleRef module, Chunk chunk, lauf_asm_signature signature) {
			return { *this, module, chunk, signature };
		}
	};

	struct AsmBuilder : UniqueHandlerDerived<AsmBuilder, AsmBuilderRef> {
		using UniqueHandlerDerived::UniqueHandlerDerived;
		using UniqueHandlerDerived::operator=;

		AsmBuilder(lauf_asm_build_options options = lauf_asm_default_build_options)
			: UniqueHandlerDerived(lauf_asm_create_builder(options)) {}

		~AsmBuilder() {
			if (_handle == nullptr) {
				return;
			}
			lauf_asm_destroy_builder(*this);
			_handle = nullptr;
		}
	};
}
