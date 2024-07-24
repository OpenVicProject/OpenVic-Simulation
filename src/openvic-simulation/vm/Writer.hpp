#pragma once

#include "Utility.hpp"
#include <lauf/writer.h>

namespace OpenVic::Vm {
	template<typename Tag>
	struct BasicWriter : utility::MoveOnlyHandleBase<BasicWriter<Tag>, lauf_writer> {
		using utility::MoveOnlyHandleBase<BasicWriter<Tag>, lauf_writer>::MoveOnlyHandleBase;
		using utility::MoveOnlyHandleBase<BasicWriter<Tag>, lauf_writer>::operator=;

		~BasicWriter() {
			if (this->_handle == nullptr) {
				return;
			}
			lauf_destroy_writer(*this);
			this->_handle = nullptr;
		}

		struct StringTag;
		struct FileTag;
		struct StdoutTag;
	};

	template<>
	struct BasicWriter<BasicWriter<void>::StringTag> : BasicWriter<void> {
		using BasicWriter<void>::BasicWriter;
		using BasicWriter<void>::operator=;

		BasicWriter() : BasicWriter(lauf_create_string_writer()) {}

		const char* c_str() {
			return lauf_writer_get_string(*this);
		}
	};

	template<>
	struct BasicWriter<BasicWriter<void>::FileTag> : BasicWriter<void> {
		using BasicWriter<void>::BasicWriter;
		using BasicWriter<void>::operator=;

		BasicWriter(const char* path) : BasicWriter(lauf_create_file_writer(path)) {}
	};

	template<>
	struct BasicWriter<BasicWriter<void>::StdoutTag> : BasicWriter<void> {
		using BasicWriter<void>::BasicWriter;
		using BasicWriter<void>::operator=;

		BasicWriter() : BasicWriter(lauf_create_stdout_writer()) {}
	};

	using StringWriter = BasicWriter<BasicWriter<void>::StringTag>;
	using FileWriter = BasicWriter<BasicWriter<void>::FileTag>;
	using CoutWriter = BasicWriter<BasicWriter<void>::StdoutTag>;
}
