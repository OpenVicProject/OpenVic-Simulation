#pragma once

#include <concepts>
#pragma comment(lib, "advapi32.lib")

#include <string>
#include <string_view>

#include <Windows.h>

#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic::Windows {
	inline memory::wstring convert(std::string_view as) {
		// deal with trivial case of empty string
		if (as.empty()) {
			return memory::wstring();
		}

		// determine required length of new string
		size_t length = ::MultiByteToWideChar(CP_UTF8, 0, as.data(), (int)as.length(), 0, 0);

		// construct new string of required length
		memory::wstring ret(length, L'\0');

		// convert old string to new string
		::MultiByteToWideChar(CP_UTF8, 0, as.data(), (int)as.length(), &ret[0], (int)ret.length());

		// return new string ( compiler should optimize this away )
		return ret;
	}

	inline memory::string convert(std::wstring_view as) {
		// deal with trivial case of empty string
		if (as.empty()) {
			return memory::string();
		}

		// determine required length of new string
		size_t length = ::WideCharToMultiByte(CP_UTF8, 0, as.data(), (int)as.length(), 0, 0, NULL, NULL);

		// construct new string of required length
		memory::string ret(length, '\0');

		// convert old string to new string
		::WideCharToMultiByte(CP_UTF8, 0, as.data(), (int)as.length(), &ret[0], (int)ret.length(), NULL, NULL);

		// return new string ( compiler should optimize this away )
		return ret;
	}

	template<typename T, typename... U>
	concept any_of = (std::same_as<T, U> || ...);

	template<typename T>
	concept either_char_type = any_of<T, char, wchar_t>;

	template<typename T>
	concept has_data = requires(T t) {
		{ t.data() } -> std::convertible_to<typename T::value_type const*>;
	};

	class RegistryKey {
	public:
		RegistryKey(HKEY key_handle) : _key_handle(key_handle) {}

		template<either_char_type CHAR_T, either_char_type CHAR_T2>
		RegistryKey(
			HKEY parent_key_handle, std::basic_string_view<CHAR_T> child_key_name, std::basic_string_view<CHAR_T2> value_name
		) {
			open_key(parent_key_handle, child_key_name);
			query_key(value_name);
		}

		~RegistryKey() {
			close_key();
		}

		bool is_open() const {
			return _key_handle != nullptr;
		}

		std::wstring_view value() const {
			return _value;
		}

		template<either_char_type CHAR_T>
		LSTATUS open_key(HKEY parent_key_handle, std::basic_string_view<CHAR_T> key_path) {
			if (is_open()) {
				close_key();
			}
			if constexpr (std::is_same_v<CHAR_T, char>) {
				return RegOpenKeyExW(parent_key_handle, convert(key_path).data(), REG_NONE, KEY_READ, &_key_handle);
			} else {
				return RegOpenKeyExW(parent_key_handle, key_path.data(), REG_NONE, KEY_READ, &_key_handle);
			}
		}

		bool is_predefined() const {
			return (_key_handle == HKEY_CURRENT_USER) || (_key_handle == HKEY_LOCAL_MACHINE) ||
				(_key_handle == HKEY_CLASSES_ROOT) || (_key_handle == HKEY_CURRENT_CONFIG) ||
				(_key_handle == HKEY_CURRENT_USER_LOCAL_SETTINGS) || (_key_handle == HKEY_PERFORMANCE_DATA) ||
				(_key_handle == HKEY_PERFORMANCE_NLSTEXT) || (_key_handle == HKEY_PERFORMANCE_TEXT) ||
				(_key_handle == HKEY_USERS);
		}

		LSTATUS close_key() {
			if (!is_open() || is_predefined()) {
				return ERROR_SUCCESS;
			}
			auto result = RegCloseKey(_key_handle);
			_key_handle = nullptr;
			return result;
		}

		template<either_char_type CHAR_T>
		LSTATUS query_key(std::basic_string_view<CHAR_T> value_name) {
			DWORD data_size;
			DWORD type;

			auto const& wide_value = [&value_name]() -> has_data auto {
				if constexpr (std::is_same_v<CHAR_T, char>) {
					return convert(value_name);
				} else {
					return value_name;
				}
			}();

			auto result = RegQueryValueExW(_key_handle, wide_value.data(), NULL, &type, NULL, &data_size);
			if (result != ERROR_SUCCESS || type != REG_SZ) {
				close_key();
				return result;
			}
			_value = memory::wstring(data_size / sizeof(wchar_t), L'\0');
			result = RegQueryValueExW(
				_key_handle, wide_value.data(), NULL, NULL, reinterpret_cast<LPBYTE>(_value.data()), &data_size
			);
			close_key();

			std::size_t first_null = _value.find_first_of(L'\0');
			if (first_null != memory::string::npos) {
				_value.resize(first_null);
			}

			return result;
		}

	private:
		HKEY _key_handle = nullptr;
		memory::wstring _value;
	};

	template<either_char_type RCHAR_T, either_char_type CHAR_T, either_char_type CHAR_T2>
	memory::basic_string<RCHAR_T> ReadRegValue( //
		HKEY root, std::basic_string_view<CHAR_T> key, std::basic_string_view<CHAR_T2> name
	) {
		RegistryKey registry_key(root, key, name);
		if constexpr (std::is_same_v<RCHAR_T, char>) {
			return convert(registry_key.value());
		} else {
			return registry_key.value();
		}
	}

	template<either_char_type RCHAR_T, either_char_type CHAR_T, either_char_type CHAR_T2>
	memory::basic_string<RCHAR_T> ReadRegValue(HKEY root, CHAR_T const* key, CHAR_T2 const* name) {
		auto key_sv = std::basic_string_view(key);
		auto name_sv = std::basic_string_view(name);

		return ReadRegValue<RCHAR_T>(root, key_sv, name_sv);
	}
}
