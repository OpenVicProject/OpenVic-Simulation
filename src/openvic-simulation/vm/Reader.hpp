#pragma once

#include <string_view>

#include "Utility.hpp"
#include <lauf/reader.h>

namespace OpenVic::Vm {
	template<typename Tag>
	struct BasicReader : utility::MoveOnlyHandleBase<BasicReader<Tag>, lauf_reader> {
		using utility::MoveOnlyHandleBase<BasicReader<Tag>, lauf_reader>::MoveOnlyHandleBase;
		using utility::MoveOnlyHandleBase<BasicReader<Tag>, lauf_reader>::operator=;

		~BasicReader() {
			if (this->_handle == nullptr) {
				return;
			}
			lauf_destroy_reader(*this);
			this->_handle = nullptr;
		}

		void set_path(const char* path) {
			lauf_reader_set_path(*this, path);
		}

		struct StringTag;
		struct FileTag;
		struct StdinTag;
	};

	template<>
	struct BasicReader<BasicReader<void>::StringTag> : BasicReader<void> {
		using BasicReader<void>::BasicReader;
		using BasicReader<void>::operator=;

		BasicReader(const char* cstr) : BasicReader(lauf_create_cstring_reader(cstr)) {}
		BasicReader(std::string_view view) : BasicReader(lauf_create_string_reader(view.data(), view.size())) {}
	};

	template<>
	struct BasicReader<BasicReader<void>::FileTag> : BasicReader<void> {
		using BasicReader<void>::BasicReader;
		using BasicReader<void>::operator=;

		BasicReader(const char* path) : BasicReader(lauf_create_file_reader(path)) {}

		bool is_valid() {
			return handle() != nullptr;
		}
	};

	template<>
	struct BasicReader<BasicReader<void>::StdinTag> : BasicReader<void> {
		using BasicReader<void>::BasicReader;
		using BasicReader<void>::operator=;

		BasicReader() : BasicReader(lauf_create_stdin_reader()) {}
	};

	using StringReader = BasicReader<BasicReader<void>::StringTag>;
	using FileReader = BasicReader<BasicReader<void>::FileTag>;
	using CinReader = BasicReader<BasicReader<void>::StdinTag>;
}
