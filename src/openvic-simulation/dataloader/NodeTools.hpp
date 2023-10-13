#pragma once

#include <map>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/Vector.hpp"

namespace OpenVic {
	namespace ast = ovdl::v2script::ast;

	/* Template for map from strings to Ts, in which string_views can be
	 * searched for without needing to be copied into a string, */
	template<typename T>
	using string_map_t = std::map<std::string, T, std::less<void>>;

	namespace NodeTools {

		template<typename... Args>
		using callback_t = std::function<bool(Args...)>;

		using node_callback_t = callback_t<ast::NodeCPtr>;
		constexpr bool success_callback(ast::NodeCPtr) { return true; }

		using key_value_callback_t = callback_t<std::string_view, ast::NodeCPtr>;
		constexpr bool key_value_success_callback(std::string_view, ast::NodeCPtr) { return true; }
		inline bool key_value_invalid_callback(std::string_view key, ast::NodeCPtr) {
			Logger::error("Invalid dictionary key: ", key);
			return false;
		}

		node_callback_t expect_identifier(callback_t<std::string_view> callback);
		node_callback_t expect_string(callback_t<std::string_view> callback, bool allow_empty = true);
		node_callback_t expect_identifier_or_string(callback_t<std::string_view> callback);

		node_callback_t expect_bool(callback_t<bool> callback);
		node_callback_t expect_int_bool(callback_t<bool> callback);

		node_callback_t expect_int64(callback_t<int64_t> callback);
		node_callback_t expect_uint64(callback_t<uint64_t> callback);

		template<std::signed_integral T>
		node_callback_t expect_int(callback_t<T> callback) {
			return expect_int64([callback](int64_t val) -> bool {
				if (static_cast<int64_t>(std::numeric_limits<T>::lowest()) <= val &&
					val <= static_cast<int64_t>(std::numeric_limits<T>::max())) {
					return callback(val);
				}
				Logger::error("Invalid int: ", val, " (valid range: [",
					static_cast<int64_t>(std::numeric_limits<T>::lowest()), ", ",
					static_cast<int64_t>(std::numeric_limits<T>::max()), "])");
				return false;
			});
		}

		template<std::integral T>
		node_callback_t expect_uint(callback_t<T> callback) {
			return expect_uint64([callback](uint64_t val) -> bool {
				if (val <= static_cast<uint64_t>(std::numeric_limits<T>::max())) {
					return callback(val);
				}
				Logger::error("Invalid uint: ", val, " (valid range: [0, ",
					static_cast<uint64_t>(std::numeric_limits<T>::max()), "])");
				return false;
			});
		}

		node_callback_t expect_fixed_point(callback_t<fixed_point_t> callback);
		node_callback_t expect_colour(callback_t<colour_t> callback);

		node_callback_t expect_date(callback_t<Date> callback);
		node_callback_t expect_years(callback_t<Timespan> callback);
		node_callback_t expect_months(callback_t<Timespan> callback);
		node_callback_t expect_days(callback_t<Timespan> callback);

		node_callback_t expect_ivec2(callback_t<ivec2_t> callback);
		node_callback_t expect_fvec2(callback_t<fvec2_t> callback);
		node_callback_t expect_assign(key_value_callback_t callback);

		using length_callback_t = std::function<size_t(size_t)>;
		constexpr size_t default_length_callback(size_t size) { return size; };

		node_callback_t expect_list_and_length(length_callback_t length_callback, node_callback_t callback);
		node_callback_t expect_list_of_length(size_t length, node_callback_t callback);
		node_callback_t expect_list(node_callback_t callback);
		node_callback_t expect_length(callback_t<size_t> callback);

		node_callback_t expect_key(std::string_view key, node_callback_t callback, bool* key_found = nullptr);

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
		using key_map_t = string_map_t<dictionary_entry_t>;

		bool add_key_map_entry(key_map_t& key_map, std::string_view key, dictionary_entry_t::expected_count_t expected_count, node_callback_t callback);
		bool remove_key_map_entry(key_map_t& key_map, std::string_view key);
		key_value_callback_t dictionary_keys_callback(key_map_t& key_map, key_value_callback_t default_callback);
		bool check_key_map_counts(key_map_t& key_map);

		constexpr bool add_key_map_entries(key_map_t& key_map) { return true; }
		template<typename... Args>
		bool add_key_map_entries(key_map_t& key_map, std::string_view key, dictionary_entry_t::expected_count_t expected_count, node_callback_t callback, Args... args) {
			bool ret = add_key_map_entry(key_map, key, expected_count, callback);
			ret &= add_key_map_entries(key_map, args...);
			return ret;
		}

		node_callback_t expect_dictionary_key_map_and_length_and_default(key_map_t key_map, length_callback_t length_callback, key_value_callback_t default_callback);
		node_callback_t expect_dictionary_key_map_and_length(key_map_t key_map, length_callback_t length_callback);
		node_callback_t expect_dictionary_key_map_and_default(key_map_t key_map, key_value_callback_t default_callback);
		node_callback_t expect_dictionary_key_map(key_map_t key_map);

		template<typename... Args>
		node_callback_t expect_dictionary_key_map_and_length_and_default(key_map_t key_map, length_callback_t length_callback, key_value_callback_t default_callback, Args... args) {
			// TODO - pass return value back up (part of big key_map_t rewrite?)
			add_key_map_entries(key_map, args...);
			return expect_dictionary_key_map_and_length_and_default(std::move(key_map), length_callback, default_callback);
		}

		template<typename... Args>
		node_callback_t expect_dictionary_keys_and_length_and_default(length_callback_t length_callback, key_value_callback_t default_callback, Args... args) {
			return expect_dictionary_key_map_and_length_and_default({}, length_callback, default_callback, args...);
		}

		template<typename... Args>
		node_callback_t expect_dictionary_keys_and_length(length_callback_t length_callback, Args... args) {
			return expect_dictionary_key_map_and_length_and_default({}, length_callback, key_value_invalid_callback, args...);
		}

		template<typename... Args>
		node_callback_t expect_dictionary_keys_and_default(key_value_callback_t default_callback, Args... args) {
			return expect_dictionary_key_map_and_length_and_default({}, default_length_callback, default_callback, args...);
		}

		template<typename... Args>
		node_callback_t expect_dictionary_keys(Args... args) {
			return expect_dictionary_key_map_and_length_and_default({}, default_length_callback, key_value_invalid_callback, args...);
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

		node_callback_t name_list_callback(callback_t<std::vector<std::string>&&> callback);

		template<typename T>
		callback_t<std::string_view> expect_mapped_string(string_map_t<T> const& map, callback_t<T> callback) {
			return [&map, callback](std::string_view string) -> bool {
				const typename string_map_t<T>::const_iterator it = map.find(string);
				if (it != map.end()) {
					return callback(it->second);
				}
				Logger::error("String not found in map: ", string);
				return false;
			};
		}

		template<typename T, typename U>
		callback_t<T> assign_variable_callback_cast(U& var) {
			return [&var](T val) -> bool {
				var = val;
				return true;
			};
		}

		template<typename T>
		callback_t<T> assign_variable_callback(T& var) {
			return assign_variable_callback_cast<T, T>(var);
		}

		callback_t<std::string_view> assign_variable_callback_string(std::string& var);

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
			t++;
		}
		key_value_callback_t increment_callback(T& var) {
			return [&var](std::string_view, ast::NodeCPtr) -> bool {
				var++;
				return true;
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
