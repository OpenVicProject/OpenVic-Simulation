#pragma once

#include <string_view>

#include <lauf/reader.h>

namespace OpenVic::Vm {
	template<typename Tag>
	struct BasicReader {
		BasicReader(BasicReader&&) = default;
		BasicReader& operator=(BasicReader&&) = default;

		BasicReader(BasicReader const&) = delete;
		BasicReader& operator=(BasicReader const&) = delete;

		~BasicReader() {
			if (_handle == nullptr) {
				return;
			}
			lauf_destroy_reader(_handle);
		}

		lauf_reader* handle() {
			return _handle;
		}

		const lauf_reader* handle() const {
			return _handle;
		}

		operator lauf_reader*() {
			return _handle;
		}

		operator const lauf_reader*() const {
			return _handle;
		}

		void set_path(const char* path) {
			lauf_reader_set_path(_handle, path);
		}

		struct StringTag;
		struct FileTag;
		struct StdinTag;

	protected:
		BasicReader(lauf_reader* reader) : _handle(reader) {}

	private:
		lauf_reader* _handle;
	};

	template<>
	struct BasicReader<BasicReader<void>::StringTag> : BasicReader<void> {
		using BasicReader<void>::BasicReader;
		BasicReader(const char* cstr) : BasicReader(lauf_create_cstring_reader(cstr)) {}
		BasicReader(std::string_view view) : BasicReader(lauf_create_string_reader(view.data(), view.size())) {}
	};

	template<>
	struct BasicReader<BasicReader<void>::FileTag> : BasicReader<void> {
		using BasicReader<void>::BasicReader;
		BasicReader(const char* path) : BasicReader(lauf_create_file_reader(path)) {}

		bool is_valid() {
			return handle() != nullptr;
		}
	};

	template<>
	struct BasicReader<BasicReader<void>::StdinTag> : BasicReader<void> {
		using BasicReader<void>::BasicReader;
		BasicReader() : BasicReader(lauf_create_stdin_reader()) {}
	};

	using StringReader = BasicReader<BasicReader<void>::StringTag>;
	using FileReader = BasicReader<BasicReader<void>::FileTag>;
	using CinReader = BasicReader<BasicReader<void>::StdinTag>;
}
