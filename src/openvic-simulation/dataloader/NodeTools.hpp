#pragma once

#include <map>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/Vector.hpp"

namespace OpenVic {
	namespace ast = ovdl::v2script::ast;

	namespace NodeTools {

		template<typename... Args>
		using callback_t = std::function<bool(Args...)>;

		using node_callback_t = callback_t<ast::NodeCPtr>;
		constexpr bool success_callback(ast::NodeCPtr) { return true; }

		using key_value_callback_t = callback_t<std::string_view, ast::NodeCPtr>;
		constexpr bool key_value_success_callback(std::string_view, ast::NodeCPtr) { return true; }

		node_callback_t expect_identifier(callback_t<std::string_view> callback);
		node_callback_t expect_string(callback_t<std::string_view> callback);
		node_callback_t expect_identifier_or_string(callback_t<std::string_view> callback);
		node_callback_t expect_bool(callback_t<bool> callback);
		node_callback_t expect_int(callback_t<int64_t> callback);
		node_callback_t expect_uint(callback_t<uint64_t> callback);
		node_callback_t expect_fixed_point(callback_t<fixed_point_t> callback);
		node_callback_t expect_colour(callback_t<colour_t> callback);
		node_callback_t expect_date(callback_t<Date> callback);
		node_callback_t expect_ivec2(callback_t<ivec2_t> callback);
		node_callback_t expect_fvec2(callback_t<fvec2_t> callback);
		node_callback_t expect_assign(key_value_callback_t callback);

		using length_callback_t = std::function<size_t(size_t)>;
		constexpr size_t default_length_callback(size_t size) { return size; };

		node_callback_t expect_list_and_length(length_callback_t length_callback, node_callback_t callback);
		node_callback_t expect_list_of_length(size_t length, node_callback_t callback);
		node_callback_t expect_list(node_callback_t callback);
		node_callback_t expect_length(callback_t<size_t> callback);

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

		void add_key_map_entry(key_map_t& key_map, std::string_view key, dictionary_entry_t::expected_count_t expected_count, node_callback_t callback);
		key_value_callback_t dictionary_keys_callback(key_map_t& key_map, bool allow_other_keys);
		bool check_key_map_counts(key_map_t const& key_map);

		constexpr struct allow_other_keys_t {} ALLOW_OTHER_KEYS;

		node_callback_t _expect_dictionary_keys_and_length(length_callback_t length_callback, bool allow_other_keys, key_map_t&& key_map);

		template<typename... Args>
		node_callback_t _expect_dictionary_keys_and_length(length_callback_t length_callback,
			bool allow_other_keys, key_map_t&& key_map,
			std::string_view key, dictionary_entry_t::expected_count_t expected_count, node_callback_t callback,
			Args... args) {
			add_key_map_entry(key_map, key, expected_count, callback);
			return _expect_dictionary_keys_and_length(length_callback, allow_other_keys, std::move(key_map), args...);
		}

		template<typename... Args>
		node_callback_t expect_dictionary_keys_and_length(length_callback_t length_callback,
			std::string_view key, dictionary_entry_t::expected_count_t expected_count, node_callback_t callback,
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
			t.reserve(size_t {});
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
		callback_t<T> assign_variable_callback(T& var) {
			return [&var](T val) -> bool {
				var = val;
				return true;
			};
		}

		template<typename T>
		callback_t<T&&> move_variable_callback(T& var) {
			return [&var](T&& val) -> bool {
				var = std::move(val);
				return true;
			};
		}

		template<typename T>
		requires requires(T& t) {
			t += T {};
		}
		callback_t<T> add_variable_callback(T& var) {
			return [&var](T val) -> bool {
				var += val;
				return true;
			};
		}

		template<typename T>
		requires requires(T& t) {
			t--;
		}
		node_callback_t decrement_callback(T& var, node_callback_t callback) {
			return [&var, callback](ast::NodeCPtr node) -> bool {
				var--;
				return callback(node);
			};
		}

		template<typename T>
		requires(std::integral<T>)
		callback_t<uint64_t> assign_variable_callback_uint(std::string_view name, T& var) {
			return [&var, name](uint64_t val) -> bool {
				if (val <= static_cast<uint64_t>(std::numeric_limits<T>::max())) {
					var = val;
					return true;
				}
				Logger::error("Invalid ", name, ": ", val, " (valid range: [0, ",
					static_cast<uint64_t>(std::numeric_limits<T>::max()), "])");
				return false;
			};
		}

		template<typename T>
		requires(std::signed_integral<T>)
		callback_t<int64_t> assign_variable_callback_int(std::string_view name, T& var) {
			return [&var, name](int64_t val) -> bool {
				if (static_cast<int64_t>(std::numeric_limits<T>::lowest()) <= val && val <= static_cast<int64_t>(std::numeric_limits<T>::max())) {
					var = val;
					return true;
				}
				Logger::error("Invalid ", name, ": ", val, " (valid range: [",
					static_cast<int64_t>(std::numeric_limits<T>::lowest()), ", ",
					static_cast<int64_t>(std::numeric_limits<T>::max()), "])");
				return false;
			};
		}

		template<typename T>
		callback_t<T const&> assign_variable_callback_pointer(T const*& var) {
			return [&var](T const& val) -> bool {
				var = &val;
				return true;
			};
		}
	}
}
