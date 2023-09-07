#pragma once

#include <map>

#include "openvic/types/Colour.hpp"
#include "openvic/types/Date.hpp"
#include "openvic/types/fixed_point/FP.hpp"

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

namespace OpenVic {
	namespace ast = ovdl::v2script::ast;

	namespace NodeTools {

		using node_callback_t = std::function<bool(ast::NodeCPtr)>;
		constexpr bool success_callback(ast::NodeCPtr) { return true; }

		using key_value_callback_t = std::function<bool(std::string_view, ast::NodeCPtr)>;

		node_callback_t expect_identifier(std::function<bool(std::string_view)> callback);
		node_callback_t expect_string(std::function<bool(std::string_view)> callback);
		node_callback_t expect_identifier_or_string(std::function<bool(std::string_view)> callback);
		node_callback_t expect_bool(std::function<bool(bool)> callback);
		node_callback_t expect_int(std::function<bool(int64_t)> callback);
		node_callback_t expect_uint(std::function<bool(uint64_t)> callback);
		node_callback_t expect_fixed_point(std::function<bool(FP)> callback);
		node_callback_t expect_colour(std::function<bool(colour_t)> callback);
		node_callback_t expect_date(std::function<bool(Date)> callback);
		node_callback_t expect_assign(key_value_callback_t callback);

		using length_callback_t = std::function<size_t(size_t)>;
		constexpr size_t default_length_callback(size_t size) { return size; };

		node_callback_t expect_list_and_length(length_callback_t length_callback, node_callback_t callback);
		node_callback_t expect_list_of_length(size_t length, node_callback_t callback);
		node_callback_t expect_list(node_callback_t callback);

		node_callback_t expect_dictionary_and_length(length_callback_t length_callback, key_value_callback_t callback);
		node_callback_t expect_dictionary(key_value_callback_t callback);

		struct dictionary_entry_t {
			const enum class expected_count_t : uint8_t {
				_MUST_APPEAR = 0b01,
				_CAN_REPEAT = 0b10,

				ZERO_OR_ONE = 0,
				ONE_EXACTLY = _MUST_APPEAR,
				ZERO_OR_MORE = _CAN_REPEAT,
				ONE_OR_MORE = _MUST_APPEAR | _CAN_REPEAT
			} expected_count;
			const node_callback_t callback;
			size_t count;

			dictionary_entry_t(expected_count_t new_expected_count, node_callback_t new_callback)
			: expected_count { new_expected_count }, callback { new_callback }, count { 0 } {}

			constexpr bool must_appear() const {
				return static_cast<uint8_t>(expected_count) & static_cast<uint8_t>(expected_count_t::_MUST_APPEAR);
			}
			constexpr bool can_repeat() const {
				return static_cast<uint8_t>(expected_count) & static_cast<uint8_t>(expected_count_t::_CAN_REPEAT);
			}
		};
		using enum dictionary_entry_t::expected_count_t;
		using key_map_t = std::map<std::string, dictionary_entry_t, std::less<void>>;

		constexpr struct allow_other_keys_t {} ALLOW_OTHER_KEYS;

		node_callback_t _expect_dictionary_keys_and_length(length_callback_t length_callback, bool allow_other_keys, key_map_t&& key_map);

		template<typename... Args>
		node_callback_t _expect_dictionary_keys_and_length(length_callback_t length_callback,
			bool allow_other_keys, key_map_t&& key_map,
			const std::string_view key, dictionary_entry_t::expected_count_t expected_count, node_callback_t callback,
			Args... args) {
			if (key_map.find(key) == key_map.end()) {
				key_map.emplace(key, dictionary_entry_t { expected_count, callback });
			} else {
				Logger::error("Duplicate expected dictionary key: ", key);
			}
			return _expect_dictionary_keys_and_length(length_callback, allow_other_keys, std::move(key_map), args...);
		}

		template<typename... Args>
		node_callback_t expect_dictionary_keys_and_length(length_callback_t length_callback,
			const std::string_view key, dictionary_entry_t::expected_count_t expected_count, node_callback_t callback,
			Args... args) {
			return _expect_dictionary_keys_and_length(length_callback, false, {}, key, expected_count, callback, args...);
		}

		template<typename... Args>
		node_callback_t expect_dictionary_keys_and_length(length_callback_t length_callback,
			allow_other_keys_t, Args... args) {
			return _expect_dictionary_keys_and_length(length_callback, true, {}, args...);
		}

		template<typename... Args>
		node_callback_t expect_dictionary_keys(Args... args) {
			return expect_dictionary_keys_and_length(default_length_callback, args...);
		}

		template<typename T>
		concept Reservable = requires(T& t) {
			{ t.size() } -> std::same_as<size_t>;
			t.reserve( size_t {} );
		};
		template<Reservable T>
		node_callback_t expect_list_reserve_length(T& t, node_callback_t callback) {
			return expect_list_and_length(
				[&t](size_t size) -> size_t {
					t.reserve(t.size() + size);
					return size;
				},
				callback
			);
		}
		template<Reservable T>
		node_callback_t expect_dictionary_reserve_length(T& t, key_value_callback_t callback) {
			return expect_list_reserve_length(t, expect_assign(callback));
		}

		node_callback_t name_list_callback(std::vector<std::string>& list);

		template<typename T>
		std::function<bool(T)> assign_variable_callback(T& var) {
			return [&var](T val) -> bool {
				var = val;
				return true;
			};
		}

		template<typename T>
		requires(std::integral<T>)
		std::function<bool(uint64_t)> assign_variable_callback_uint(const std::string_view name, T& var) {
			return [&var, name](uint64_t val) -> bool {
				if (val <= std::numeric_limits<T>::max()) {
					var = val;
					return true;
				}
				Logger::error("Invalid ", name, ": ", val, " (valid range: [0, ", static_cast<uint64_t>(std::numeric_limits<T>::max()), "])");
				return false;
			};
		}

		template<typename T>
		requires(std::integral<T>)
		std::function<bool(int64_t)> assign_variable_callback_int(const std::string_view name, T& var) {
			return [&var, name](int64_t val) -> bool {
				if (std::numeric_limits<T>::lowest() <= val && val <= std::numeric_limits<T>::max()) {
					var = val;
					return true;
				}
				Logger::error("Invalid ", name, ": ", val, " (valid range: [",
					static_cast<int64_t>(std::numeric_limits<T>::lowest()), ", ",
					static_cast<uint64_t>(std::numeric_limits<T>::max()), "])");
				return false;
			};
		}
	}
}
