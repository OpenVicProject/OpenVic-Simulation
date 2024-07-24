#pragma once

#include <lauf/writer.h>

namespace OpenVic::Vm {
	template<typename Tag>
	struct BasicWriter {
		BasicWriter(BasicWriter&&) = default;
		BasicWriter& operator=(BasicWriter&&) = default;

		BasicWriter(BasicWriter const&) = delete;
		BasicWriter& operator=(BasicWriter const&) = delete;

		~BasicWriter() {
			lauf_destroy_writer(_handle);
		}

		lauf_writer* handle() {
			return _handle;
		}

		const lauf_writer* handle() const {
			return _handle;
		}

		operator lauf_writer*() {
			return _handle;
		}

		operator const lauf_writer*() const {
			return _handle;
		}

		struct StringTag;
		struct FileTag;
		struct StdoutTag;

	protected:
		BasicWriter(lauf_writer* writer) : _handle(writer) {}

	private:
		lauf_writer* _handle;
	};

	template<>
	struct BasicWriter<BasicWriter<void>::StringTag> : BasicWriter<void> {
		using BasicWriter<void>::BasicWriter;
		BasicWriter() : BasicWriter(lauf_create_string_writer()) {}

		const char* c_str() {
			return lauf_writer_get_string(*this);
		}
	};

	template<>
	struct BasicWriter<BasicWriter<void>::FileTag> : BasicWriter<void> {
		using BasicWriter<void>::BasicWriter;
		BasicWriter(const char* path) : BasicWriter(lauf_create_file_writer(path)) {}
	};

	template<>
	struct BasicWriter<BasicWriter<void>::StdoutTag> : BasicWriter<void> {
		using BasicWriter<void>::BasicWriter;
		BasicWriter() : BasicWriter(lauf_create_stdout_writer()) {}
	};

	using StringWriter = BasicWriter<BasicWriter<void>::StringTag>;
	using FileWriter = BasicWriter<BasicWriter<void>::FileTag>;
	using CoutWriter = BasicWriter<BasicWriter<void>::StdoutTag>;
}
