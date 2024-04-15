#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <tsl/ordered_set.h>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/utility/TslHelper.hpp"

namespace OpenVic {
	namespace ast = ovdl::v2script::ast;

	using name_list_t = std::vector<std::string>;
	std::ostream& operator<<(std::ostream& stream, name_list_t const& name_list);

	template<typename T>
	concept Reservable = requires(T& t, size_t size) {
		{ t.size() } -> std::same_as<size_t>;
		t.reserve(size);
	};
	constexpr void reserve_more(Reservable auto& t, size_t size) {
		t.reserve(t.size() + size);
	}

	namespace NodeTools {

		template<typename Fn, typename Return = void, typename... Args>
		concept Functor = requires(Fn&& fn, Args&&... args) {
			{ std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...) } -> std::same_as<Return>;
		};

		template<typename Fn, typename Return = void, typename... Args>
		concept FunctorConvertible = requires(Fn&& fn, Args&&... args) {
			{ std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...) } -> std::convertible_to<Return>;
		};

		template<typename Fn, typename... Args>
		concept Callback = Functor<Fn, bool, Args...>;

		template<typename Fn>
		concept NodeCallback = Callback<Fn, ast::NodeCPtr>;

		template<typename Fn>
		concept KeyValueCallback = Callback<Fn, std::string_view, ast::NodeCPtr>;

		template<typename Fn>
		concept LengthCallback = Functor<Fn, std::size_t, std::size_t>;

		template<typename... Args>
		using callback_t = std::function<bool(Args...)>;

		using node_callback_t = callback_t<ast::NodeCPtr>;
		constexpr bool success_callback(ast::NodeCPtr) {
			return true;
		}

		using key_value_callback_t = callback_t<std::string_view, ast::NodeCPtr>;
		constexpr bool key_value_success_callback(std::string_view, ast::NodeCPtr) {
			return true;
		}
		inline bool key_value_invalid_callback(std::string_view key, ast::NodeCPtr) {
			Logger::error("Invalid dictionary key: ", key);
			return false;
		}

		node_callback_t expect_identifier(callback_t<std::string_view> callback);
		node_callback_t expect_string(callback_t<std::string_view> callback, bool allow_empty = false);
		node_callback_t expect_identifier_or_string(callback_t<std::string_view> callback, bool allow_empty = false);

		node_callback_t expect_bool(callback_t<bool> callback);
		node_callback_t expect_int_bool(callback_t<bool> callback);

		node_callback_t expect_int64(callback_t<int64_t> callback, int base = 10);
		node_callback_t expect_uint64(callback_t<uint64_t> callback, int base = 10);

		template<std::signed_integral T>
		NodeCallback auto expect_int(callback_t<T> callback, int base = 10) {
			return expect_int64([callback](int64_t val) -> bool {
				if (static_cast<int64_t>(std::numeric_limits<T>::lowest()) <= val &&
					val <= static_cast<int64_t>(std::numeric_limits<T>::max())) {
					return callback(val);
				}
				Logger::error(
					"Invalid int: ", val, " (valid range: [", static_cast<int64_t>(std::numeric_limits<T>::lowest()), ", ",
					static_cast<int64_t>(std::numeric_limits<T>::max()), "])"
				);
				return false;
			}, base);
		}

		template<std::integral T>
		NodeCallback auto expect_uint(callback_t<T> callback, int base = 10) {
			return expect_uint64([callback](uint64_t val) -> bool {
				if (val <= static_cast<uint64_t>(std::numeric_limits<T>::max())) {
					return callback(val);
				}
				Logger::error(
					"Invalid uint: ", val, " (valid range: [0, ", static_cast<uint64_t>(std::numeric_limits<T>::max()), "])"
				);
				return false;
			}, base);
		}

		callback_t<std::string_view> expect_fixed_point_str(callback_t<fixed_point_t> callback);
		node_callback_t expect_fixed_point(callback_t<fixed_point_t> callback);
		/* Expect a list of 3 base 10 values, each either in the range [0, 1] or (1, 255], representing RGB components. */
		node_callback_t expect_colour(callback_t<colour_t> callback);
		/* Expect a hexadecimal value representing a colour in ARGB format. */
		node_callback_t expect_colour_hex(callback_t<colour_argb_t> callback);

		callback_t<std::string_view> expect_date_str(callback_t<Date> callback);
		node_callback_t expect_date(callback_t<Date> callback);
		node_callback_t expect_date_string(callback_t<Date> callback);
		node_callback_t expect_date_identifier_or_string(callback_t<Date> callback);
		node_callback_t expect_years(callback_t<Timespan> callback);
		node_callback_t expect_months(callback_t<Timespan> callback);
		node_callback_t expect_days(callback_t<Timespan> callback);

		node_callback_t expect_ivec2(callback_t<ivec2_t> callback);
		node_callback_t expect_fvec2(callback_t<fvec2_t> callback);
		node_callback_t expect_assign(key_value_callback_t callback);

		using length_callback_t = std::function<size_t(size_t)>;
		constexpr size_t default_length_callback(size_t size) {
			return size;
		};

		node_callback_t expect_list_and_length(length_callback_t length_callback, node_callback_t callback);
		node_callback_t expect_list_of_length(size_t length, node_callback_t callback);
		node_callback_t expect_list(node_callback_t callback);
		node_callback_t expect_length(callback_t<size_t> callback);

		node_callback_t expect_key(
			std::string_view key, node_callback_t callback, bool* key_found = nullptr, bool allow_duplicates = false
		);

		node_callback_t expect_dictionary_and_length(length_callback_t length_callback, key_value_callback_t callback);
		node_callback_t expect_dictionary(key_value_callback_t callback);

		struct dictionary_entry_t {
			enum class expected_count_t : uint8_t {
				_MUST_APPEAR = 0b01,
				_CAN_REPEAT = 0b10,

				ZERO_OR_ONE = 0,
				ONE_EXACTLY = _MUST_APPEAR,
				ZERO_OR_MORE = _CAN_REPEAT,
				ONE_OR_MORE = _MUST_APPEAR | _CAN_REPEAT
			} expected_count;
			node_callback_t callback;
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

		template<StringMapCase Case>
		using template_key_map_t = template_string_map_t<dictionary_entry_t, Case>;

		using key_map_t = template_key_map_t<StringMapCaseSensitive>;
		using case_insensitive_key_map_t = template_key_map_t<StringMapCaseInsensitive>;

		template<StringMapCase Case>
		bool add_key_map_entry(
			template_key_map_t<Case>& key_map, std::string_view key, dictionary_entry_t::expected_count_t expected_count,
			NodeCallback auto callback
		) {
			if (!key_map.contains(key)) {
				key_map.emplace(key, dictionary_entry_t { expected_count, callback });
				return true;
			}
			Logger::error("Duplicate expected dictionary key: ", key);
			return false;
		}

		template<StringMapCase Case>
		bool remove_key_map_entry(template_key_map_t<Case>& key_map, std::string_view key) {
			if (key_map.erase(key) == 0) {
				Logger::error("Failed to find dictionary key to remove: ", key);
				return false;
			}
			return true;
		}

		template<StringMapCase Case>
		KeyValueCallback auto dictionary_keys_callback(
			template_key_map_t<Case>& key_map, KeyValueCallback auto default_callback
		) {
			return [&key_map, default_callback](std::string_view key, ast::NodeCPtr value) -> bool {
				typename template_key_map_t<Case>::iterator it = key_map.find(key);
				if (it == key_map.end()) {
					return default_callback(key, value);
				}
				dictionary_entry_t& entry = it.value();
				if (++entry.count > 1 && !entry.can_repeat()) {
					Logger::error("Invalid repeat of dictionary key: ", key);
					return false;
				}
				if (entry.callback(value)) {
					return true;
				} else {
					Logger::error("Callback failed for dictionary key: ", key);
					return false;
				}
			};
		}

		template<StringMapCase Case>
		bool check_key_map_counts(template_key_map_t<Case>& key_map) {
			bool ret = true;
			for (auto key_entry : mutable_iterator(key_map)) {
				dictionary_entry_t& entry = key_entry.second;
				if (entry.must_appear() && entry.count < 1) {
					Logger::error("Mandatory dictionary key not present: ", key_entry.first);
					ret = false;
				}
				entry.count = 0;
			}
			return ret;
		}

		template<StringMapCase Case>
		constexpr bool add_key_map_entries(template_key_map_t<Case>& key_map) {
			return true;
		}

		template<StringMapCase Case, typename... Args>
		bool add_key_map_entries(
			template_key_map_t<Case>& key_map, std::string_view key, dictionary_entry_t::expected_count_t expected_count,
			NodeCallback auto callback, Args... args
		) {
			bool ret = add_key_map_entry(key_map, key, expected_count, callback);
			ret &= add_key_map_entries(key_map, args...);
			return ret;
		}

		template<StringMapCase Case>
		NodeCallback auto expect_dictionary_key_map_and_length_and_default(
			template_key_map_t<Case> key_map, LengthCallback auto length_callback, KeyValueCallback auto default_callback
		) {
			return [length_callback, default_callback, key_map = std::move(key_map)](ast::NodeCPtr node) mutable -> bool {
				bool ret = expect_dictionary_and_length(
					length_callback, dictionary_keys_callback(key_map, default_callback)
				)(node);
				ret &= check_key_map_counts(key_map);
				return ret;
			};
		}

		template<StringMapCase Case>
		NodeCallback auto expect_dictionary_key_map_and_length(
			template_key_map_t<Case> key_map, LengthCallback auto length_callback
		) {
			return expect_dictionary_key_map_and_length_and_default(
				std::move(key_map), length_callback, key_value_invalid_callback
			);
		}

		template<StringMapCase Case>
		NodeCallback auto expect_dictionary_key_map_and_default(
			template_key_map_t<Case> key_map, KeyValueCallback auto default_callback
		) {
			return expect_dictionary_key_map_and_length_and_default(
				std::move(key_map), default_length_callback, default_callback
			);
		}

		template<StringMapCase Case>
		NodeCallback auto expect_dictionary_key_map(template_key_map_t<Case> key_map) {
			return expect_dictionary_key_map_and_length_and_default(
				std::move(key_map), default_length_callback, key_value_invalid_callback
			);
		}

		template<StringMapCase Case, typename... Args>
		NodeCallback auto expect_dictionary_key_map_and_length_and_default(
			template_key_map_t<Case> key_map, LengthCallback auto length_callback, KeyValueCallback auto default_callback,
			Args... args
		) {
			// TODO - pass return value back up (part of big key_map_t rewrite?)
			add_key_map_entries(key_map, args...);
			return expect_dictionary_key_map_and_length_and_default(std::move(key_map), length_callback, default_callback);
		}

		template<StringMapCase Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_and_length_and_default(
			LengthCallback auto length_callback, KeyValueCallback auto default_callback, Args... args
		) {
			return expect_dictionary_key_map_and_length_and_default(
				template_key_map_t<Case> {}, length_callback, default_callback, args...
			);
		}

		template<StringMapCase Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_and_length(LengthCallback auto length_callback, Args... args) {
			return expect_dictionary_key_map_and_length_and_default(
				template_key_map_t<Case> {}, length_callback, key_value_invalid_callback, args...
			);
		}

		template<StringMapCase Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_and_default(KeyValueCallback auto default_callback, Args... args) {
			return expect_dictionary_key_map_and_length_and_default(
				template_key_map_t<Case> {}, default_length_callback, default_callback, args...
			);
		}

		template<StringMapCase Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys(Args... args) {
			return expect_dictionary_key_map_and_length_and_default(
				template_key_map_t<Case> {}, default_length_callback, key_value_invalid_callback, args...
			);
		}

		LengthCallback auto reserve_length_callback(Reservable auto& reservable) {
			return [&reservable](size_t size) -> size_t {
				reserve_more(reservable, size);
				return size;
			};
		}
		NodeCallback auto expect_list_reserve_length(Reservable auto& reservable, NodeCallback auto callback) {
			return expect_list_and_length(reserve_length_callback(reservable), callback);
		}
		NodeCallback auto expect_dictionary_reserve_length(Reservable auto& reservable, KeyValueCallback auto callback) {
			return expect_dictionary_and_length(reserve_length_callback(reservable), callback);
		}
		template<StringMapCase Case, typename... Args>
		NodeCallback auto expect_dictionary_key_map_reserve_length_and_default(
			Reservable auto& reservable, template_key_map_t<Case> key_map, KeyValueCallback auto default_callback,
			Args... args
		) {
			return expect_dictionary_key_map_and_length_and_default(
				std::move(key_map), reserve_length_callback(reservable), default_callback, args...
			);
		}
		template<StringMapCase Case, typename... Args>
		NodeCallback auto expect_dictionary_key_map_reserve_length(
			Reservable auto& reservable, template_key_map_t<Case> key_map, Args... args
		) {
			return expect_dictionary_key_map_and_length(std::move(key_map), reserve_length_callback(reservable), args...);
		}
		template<StringMapCase Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_reserve_length_and_default(
			Reservable auto& reservable, KeyValueCallback auto default_callback, Args... args
		) {
			return expect_dictionary_keys_and_length_and_default<Case>(
				reserve_length_callback(reservable), default_callback, args...
			);
		}
		template<StringMapCase Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_reserve_length(Reservable auto& reservable, Args... args) {
			return expect_dictionary_keys_and_length<Case>(reserve_length_callback(reservable), args...);
		}

		node_callback_t name_list_callback(callback_t<name_list_t&&> callback);

		template<typename T, StringMapCase Case>
		Callback<std::string_view> auto expect_mapped_string(
			template_string_map_t<T, Case> const& map, Callback<T> auto callback, bool warn = false
		) {
			return [&map, callback, warn](std::string_view string) -> bool {
				const typename template_string_map_t<T, Case>::const_iterator it = map.find(string);
				if (it != map.end()) {
					return callback(it->second);
				}
				Logger::warn_or_error(warn, "String not found in map: ", string);
				return warn;
			};
		}

		template<typename T>
		Callback<T> auto assign_variable_callback_cast(auto& var) {
			return [&var](T val) -> bool {
				var = val;
				return true;
			};
		}

		template<typename T>
		requires std::is_integral_v<T> || std::is_enum_v<T>
		callback_t<T> assign_variable_callback_cast(auto& var) {
			return [&var](T val) -> bool {
				var = val;
				return true;
			};
		}

		template<typename T>
		Callback<T> auto assign_variable_callback(T& var) {
			return assign_variable_callback_cast<T, T>(var);
		}

		callback_t<std::string_view> assign_variable_callback_string(std::string& var);

		template<typename T>
		Callback<T&&> auto move_variable_callback(T& var) {
			return [&var](T&& val) -> bool {
				var = std::move(val);
				return true;
			};
		}

		template<typename T>
		requires requires(T& t) { t += T {}; }
		Callback<T> auto add_variable_callback(T& var) {
			return [&var](T val) -> bool {
				var += val;
				return true;
			};
		}

		template<typename T>
		requires requires(T& t) { t++; }
		KeyValueCallback auto increment_callback(T& var) {
			return [&var](std::string_view, ast::NodeCPtr) -> bool {
				var++;
				return true;
			};
		}

		template<typename T>
		Callback<T const&> auto assign_variable_callback_pointer(T const*& var) {
			return [&var](T const& val) -> bool {
				var = &val;
				return true;
			};
		}

		/* By default this will only allow an optional to be set once. Set allow_overwrite
		 * to true to allow multiple assignments, with the last taking precedence. */
		template<typename T>
		Callback<T const&> auto assign_variable_callback_pointer_opt(
			std::optional<T const*>& var, bool allow_overwrite = false
		) {
			return [&var, allow_overwrite](T const& val) -> bool {
				if (!allow_overwrite && var.has_value()) {
					Logger::error("Canoot assign pointer value to already-initialised optional!");
					return false;
				}
				var = &val;
				return true;
			};
		}

		template<typename T, typename U>
		Callback<T> auto vector_callback(std::vector<U>& vec) {
			return [&vec](T val) -> bool {
				vec.emplace_back(std::move(val));
				return true;
			};
		}

		template<typename T>
		Callback<T> auto vector_callback(std::vector<T>& vec) {
			return vector_callback<T, T>(vec);
		}

		template<typename T>
		Callback<T const&> auto vector_callback_pointer(std::vector<T const*>& vec) {
			return [&vec](T const& val) -> bool {
				vec.emplace_back(&val);
				return true;
			};
		}

		template<typename T, typename U, typename... SetArgs>
		Callback<T> auto set_callback(tsl::ordered_set<U, SetArgs...>& set, bool warn = false) {
			return [&set, warn](T val) -> bool {
				if (set.emplace(std::move(val)).second) {
					return true;
				}
				Logger::warn_or_error(warn, "Duplicate set entry: \"", val, "\"");
				return warn;
			};
		}

		template<std::derived_from<HasIdentifier> T, typename... SetArgs>
		Callback<T const&> auto set_callback_pointer(tsl::ordered_set<T const*, SetArgs...>& set, bool warn = false) {
			return [&set, warn](T const& val) -> bool {
				if (set.emplace(&val).second) {
					return true;
				}
				Logger::warn_or_error(warn, "Duplicate set entry: \"", &val, "\"");
				return warn;
			};
		}

		template<std::derived_from<HasIdentifier> Key, typename Value, typename... MapArgs>
		Callback<Value> auto map_callback(
			tsl::ordered_map<Key const*, Value, MapArgs...>& map, Key const* key, bool warn = false
		) {
			return [&map, key, warn](Value value) -> bool {
				if (map.emplace(key, std::move(value)).second) {
					return true;
				}
				Logger::warn_or_error(warn, "Duplicate map entry with key: \"", key, "\"");
				return warn;
			};
		}

		/* Often used for rotations which must be negated due to OpenVic's coordinate system being orientated
		 * oppositely to Vic2's. */
		template<typename T = fixed_point_t>
		constexpr Callback<T> auto negate_callback(Callback<T> auto callback) {
			return [callback](T val) -> bool {
				return callback(-val);
			};
		}

		/* Often used for map-space coordinates which must have their y-coordinate flipped due to OpenVic using the
		 * top-left of the map as the origin as opposed Vic2 using the bottom-left. */
		template<typename T>
		constexpr Callback<vec2_t<T>> auto flip_y_callback(Callback<vec2_t<T>> auto callback, T height) {
			return [callback, height](vec2_t<T> val) -> bool {
				val.y = height - val.y;
				return callback(val);
			};
		}
	}
}
