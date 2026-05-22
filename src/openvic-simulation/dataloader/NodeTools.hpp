#pragma once

#include <concepts>
#include <cstdint>
#include <optional>
#include <type_traits>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <function2/function2.hpp>

#include <spdlog/common.h>

#include <tsl/ordered_set.h>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/core/FormatValidate.hpp"
#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/core/stl/MutableIterator.hpp"
#include "openvic-simulation/core/string/Utility.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/core/template/FunctionalConcepts.hpp"
#include "openvic-simulation/core/ui/TextFormat.hpp"
#include "openvic-simulation/dataloader/NodeCallbacks.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/Vector.hpp"

#define MOV(...) static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define FWD(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace OpenVic {
	namespace ast {
		static constexpr std::string_view get_type_name(ovdl::v2script::ast::NodeKind kind) {
#ifdef _MSC_VER // type_name starts with "struct "
using namespace std::string_view_literals;
#define NODE_CASE(Node) \
	case Node: return OpenVic::type_name<ovdl::v2script::ast::Node>().substr("struct "sv.size());
#else
#define NODE_CASE(Node) \
	case Node: return OpenVic::type_name<ovdl::v2script::ast::Node>();
#endif
			switch (kind) {
				using enum ovdl::v2script::ast::NodeKind;
				NODE_CASE(FileTree);
				NODE_CASE(IdentifierValue);
				NODE_CASE(StringValue);
				NODE_CASE(ListValue);
				NODE_CASE(NullValue);
				NODE_CASE(EventStatement);
				NODE_CASE(AssignStatement);
				NODE_CASE(ValueStatement);
			default: ovdl::detail::unreachable();
			}
		}
#undef NODE_CASE
	}

	std::ostream& operator<<(std::ostream& stream, memory::vector<memory::string> const& name_list);

	constexpr void reserve_more(reservable auto& t, size_t size) {
		t.reserve(t.size() + size);
	}

	namespace NodeTools {
		template<typename Map>
		constexpr static MapKeyValueCallback<Map> auto ignore_map(KeyValueCallback auto&& callback) {
			return [default_callback = MOV(callback)](
				Map const& key_map,
				std::string_view key,
				ovdl::v2script::ast::Node const* value
			) mutable -> bool {
				return default_callback(key, value);
			};
		}

		template<typename Fn>
		concept LengthCallback = Functor<Fn, std::size_t, std::size_t>;

		constexpr bool success_callback(ovdl::v2script::ast::Node const*) {
			return true;
		}

		using key_value_callback_t = NodeTools::callback_t<std::string_view, ovdl::v2script::ast::Node const*>;
		constexpr bool key_value_success_callback(std::string_view, ovdl::v2script::ast::Node const*) {
			return true;
		}
		inline bool key_value_warn_callback(std::string_view key, ovdl::v2script::ast::Node const*) {
			spdlog::warn_s("Invalid dictionary key: {}", key);
			return true;
		}
		inline bool key_value_invalid_callback(std::string_view key, ovdl::v2script::ast::Node const*) {
			spdlog::error_s("Invalid dictionary key: {}", key);
			return false;
		}

		template<derived_ordered_map Map>
		inline bool map_key_value_invalid_callback(Map const& key_map, std::string_view key, ovdl::v2script::ast::Node const*) {
			spdlog::error_s(
				"Invalid dictionary key \"{}\". Valid values are [{}]",
				key, string_join(key_map)
			);
			return false;
		}

		template<derived_ordered_map Map>
		inline bool map_key_value_ignore_invalid_callback(Map const& key_map, std::string_view key, ovdl::v2script::ast::Node const*) {
			spdlog::warn_s(
				"Invalid dictionary key \"{}\" is ignored. Valid values are [{}]",
				key, string_join(key_map)
			);
			return true;
		}

		NodeTools::node_callback_t expect_identifier(NodeTools::callback_t<std::string_view> callback);
		NodeTools::node_callback_t expect_string(NodeTools::callback_t<std::string_view> callback, bool allow_empty = false);
		NodeTools::node_callback_t expect_identifier_or_string(NodeTools::callback_t<std::string_view> callback, bool allow_empty = false);

		NodeTools::node_callback_t expect_bool(NodeTools::callback_t<bool> callback);
		NodeTools::node_callback_t expect_int_bool(NodeTools::callback_t<bool> callback);

		NodeTools::node_callback_t expect_int64(NodeTools::callback_t<int64_t> callback, int base = 10);
		NodeTools::node_callback_t expect_uint64(NodeTools::callback_t<uint64_t> callback, int base = 10);

		template<std::signed_integral T>
		NodeCallback auto expect_int(NodeTools::callback_t<T>& callback, int base = 10) {
			return expect_int64([callback](int64_t val) mutable -> bool {
				if (static_cast<int64_t>(std::numeric_limits<T>::lowest()) <= val
					&& val <= static_cast<int64_t>(std::numeric_limits<T>::max())
				) {
					return callback(val);
				}
				spdlog::error_s(
					"Invalid int: {} (valid range: [{}, {}])",
					val,
					static_cast<int64_t>(std::numeric_limits<T>::lowest()),
					static_cast<int64_t>(std::numeric_limits<T>::max())
				);
				return false;
			}, base);
		}
		template<std::signed_integral T>
		NodeCallback auto expect_int(NodeTools::callback_t<T>&& callback, int base = 10) {
			return expect_int(callback, base);
		}

		template<std::integral T>
		NodeCallback auto expect_uint(NodeTools::callback_t<T>& callback, int base = 10) {
			return expect_uint64([callback](uint64_t val) mutable -> bool {
				if (val <= static_cast<uint64_t>(std::numeric_limits<T>::max())) {
					return callback(val);
				}
				spdlog::error_s(
					"Invalid uint: {} (valid range: [0, {}])",
					val, static_cast<uint64_t>(std::numeric_limits<T>::max())
				);
				return false;
			}, base);
		}
		template<std::integral T>
		NodeCallback auto expect_uint(NodeTools::callback_t<T>&& callback, int base = 10) {
			return expect_uint(callback, base);
		}

		template<derived_from_specialization_of<type_safe::strong_typedef> T, typename AsT = type_safe::underlying_type<T>>
		NodeCallback auto expect_strong_typedef(NodeTools::callback_t<T>& callback, int base = 10) {
			if constexpr (std::unsigned_integral<AsT>) {
				return expect_uint64(
					[callback](uint64_t val) mutable -> bool {
						if (val <= static_cast<uint64_t>(std::numeric_limits<AsT>::max())) {
							return callback(T(val));
						}
						spdlog::error_s(
							"Invalid uint: {} (valid range: [0, {}])", val,
							static_cast<uint64_t>(std::numeric_limits<AsT>::max())
						);
						return false;
					},
					base
				);
			} else {
				return expect_int64(
					[callback](int64_t val) mutable -> bool {
						if (val >= static_cast<int64_t>(std::numeric_limits<AsT>::min()) &&
							val <= static_cast<int64_t>(std::numeric_limits<AsT>::max())) {
							return callback(T(val));
						}
						spdlog::error_s(
							"Invalid int: {} (valid range: [{}, {}])", val,
							static_cast<int64_t>(std::numeric_limits<AsT>::min()),
							static_cast<int64_t>(std::numeric_limits<AsT>::max())
						);
						return false;
					},
					base
				);
			}
		}
		template<derived_from_specialization_of<type_safe::strong_typedef> T, typename AsT = type_safe::underlying_type<T>>
		NodeCallback auto expect_strong_typedef(NodeTools::callback_t<T>&& callback, int base = 10) {
			return expect_strong_typedef<T, AsT>(callback, base);
		}

		template<derived_from_specialization_of<type_safe::strong_typedef> T>
		requires std::unsigned_integral<type_safe::underlying_type<T>>
		NodeCallback auto expect_index(NodeTools::callback_t<T>& callback, int base = 10) {
			return expect_strong_typedef<T>(callback, base);
		}
		template<derived_from_specialization_of<type_safe::strong_typedef> T>
		requires std::unsigned_integral<type_safe::underlying_type<T>>
		NodeCallback auto expect_index(NodeTools::callback_t<T>&& callback, int base = 10) {
			return expect_index(callback, base);
		}

		NodeTools::callback_t<std::string_view> expect_fixed_point_str(NodeTools::callback_t<fixed_point_t> callback);
		NodeTools::node_callback_t expect_fixed_point(NodeTools::callback_t<fixed_point_t> callback);
		/* Expect a list of 3 base 10 values, each either in the range [0, 1] or (1, 255], representing RGB components. */
		NodeTools::node_callback_t expect_colour(NodeTools::callback_t<colour_t> callback);
		/* Expect a hexadecimal value representing a colour in ARGB format. */
		NodeTools::node_callback_t expect_colour_hex(NodeTools::callback_t<colour_argb_t> callback);

		NodeTools::node_callback_t expect_text_format(NodeTools::callback_t<text_format_t> callback);

		NodeTools::callback_t<std::string_view> expect_date_str(NodeTools::callback_t<Date> callback);
		NodeTools::node_callback_t expect_date(NodeTools::callback_t<Date> callback);
		NodeTools::node_callback_t expect_date_string(NodeTools::callback_t<Date> callback);
		NodeTools::node_callback_t expect_date_identifier_or_string(NodeTools::callback_t<Date> callback);
		NodeTools::node_callback_t expect_years(NodeTools::callback_t<Timespan> callback);
		NodeTools::node_callback_t expect_months(NodeTools::callback_t<Timespan> callback);
		NodeTools::node_callback_t expect_days(NodeTools::callback_t<Timespan> callback);

		NodeTools::node_callback_t expect_ivec2(NodeTools::callback_t<ivec2_t> callback);
		NodeTools::node_callback_t expect_fvec2(NodeTools::callback_t<fvec2_t> callback);
		NodeTools::node_callback_t expect_fvec3(NodeTools::callback_t<fvec3_t> callback);
		NodeTools::node_callback_t expect_fvec4(NodeTools::callback_t<fvec4_t> callback);
		NodeTools::node_callback_t expect_assign(key_value_callback_t callback);

		using length_callback_t = fu2::function<size_t(size_t)>;
		constexpr size_t default_length_callback(size_t size) {
			return size;
		};

		NodeTools::node_callback_t expect_list_and_length(length_callback_t length_callback, NodeTools::node_callback_t callback);
		NodeTools::node_callback_t expect_list_of_length(size_t length, NodeTools::node_callback_t callback);
		NodeTools::node_callback_t expect_list(NodeTools::node_callback_t callback);
		NodeTools::node_callback_t expect_length(NodeTools::callback_t<size_t> callback);

		NodeTools::node_callback_t expect_key(
			ovdl::symbol<char> key, NodeTools::node_callback_t callback, bool* key_found = nullptr, bool allow_duplicates = false
		);

		NodeTools::node_callback_t expect_key(
			std::string_view key, NodeTools::node_callback_t callback, bool* key_found = nullptr, bool allow_duplicates = false
		);

		NodeTools::node_callback_t expect_dictionary_and_length(length_callback_t length_callback, key_value_callback_t callback);
		NodeTools::node_callback_t expect_dictionary(key_value_callback_t callback);

		struct dictionary_entry_t {
			enum class expected_count_t : uint8_t {
				_MUST_APPEAR = 0b01,
				_CAN_REPEAT = 0b10,

				ZERO_OR_ONE = 0,
				ONE_EXACTLY = _MUST_APPEAR,
				ZERO_OR_MORE = _CAN_REPEAT,
				ONE_OR_MORE = _MUST_APPEAR | _CAN_REPEAT
			} expected_count;
			NodeTools::node_callback_t callback;
			size_t count = 0;

			dictionary_entry_t(expected_count_t new_expected_count, NodeTools::node_callback_t&& new_callback)
				: expected_count { new_expected_count }, callback { MOV(new_callback) } {}

			constexpr bool must_appear() const {
				return static_cast<uint8_t>(expected_count) & static_cast<uint8_t>(expected_count_t::_MUST_APPEAR);
			}
			constexpr bool can_repeat() const {
				return static_cast<uint8_t>(expected_count) & static_cast<uint8_t>(expected_count_t::_CAN_REPEAT);
			}
		};
		using enum dictionary_entry_t::expected_count_t;

		template<string_map_case Case>
		using template_key_map_t = template_string_map_t<dictionary_entry_t, Case>;

		using key_map_t = template_key_map_t<StringMapCaseSensitive>;
		using case_insensitive_key_map_t = template_key_map_t<StringMapCaseInsensitive>;

		template<derived_ordered_map Map>
		bool add_key_map_entry(
			Map&& key_map, std::string_view key, dictionary_entry_t::expected_count_t expected_count,
			NodeCallback auto&& callback
		) {
			if (!key_map.contains(key)) {
				key_map.emplace(key, dictionary_entry_t { expected_count, MOV(callback) });
				return true;
			}
			spdlog::error_s("Duplicate expected dictionary key: {}", key);
			return false;
		}

		template<derived_ordered_map Map>
		bool remove_key_map_entry(Map&& key_map, std::string_view key) {
			if (key_map.erase(key) == 0) {
				spdlog::error_s("Failed to find dictionary key to remove: {}", key);
				return false;
			}
			return true;
		}

		template<derived_ordered_map Map>
		KeyValueCallback auto dictionary_keys_callback(
			Map&& key_map, MapKeyValueCallback<Map> auto&& default_callback
		) {
			return [&key_map, default_callback = FWD(default_callback)](std::string_view key, ovdl::v2script::ast::Node const* value) mutable -> bool {
				typename std::remove_reference_t<Map>::iterator it = key_map.find(key);
				if (it == key_map.end()) {
					return default_callback(key_map, key, value);
				}
				dictionary_entry_t& entry = it.value();
				if (++entry.count > 1 && !entry.can_repeat()) {
					spdlog::error_s("Invalid repeat of dictionary key: {}", key);
					return false;
				}
				if (entry.callback(value)) {
					return true;
				} else {
					spdlog::error_s("Callback failed for dictionary key: {}", key);
					return false;
				}
			};
		}

		template<derived_ordered_map Map>
		bool check_key_map_counts(Map&& key_map) {
			bool ret = true;
			for (auto key_entry : mutable_iterator(key_map)) {
				dictionary_entry_t& entry = key_entry.second;
				if (entry.must_appear() && entry.count < 1) {
					spdlog::error_s("Mandatory dictionary key not present: {}", key_entry.first);
					ret = false;
				}
				entry.count = 0;
			}
			return ret;
		}

		template<derived_ordered_map Map>
		constexpr bool add_key_map_entries(Map&& key_map) {
			return true;
		}

		template<derived_ordered_map Map, typename... Args>
		bool add_key_map_entries(
			Map&& key_map, std::string_view key, dictionary_entry_t::expected_count_t expected_count,
			NodeCallback auto&& callback, Args&&... args
		) {
			bool ret = add_key_map_entry(FWD(key_map), FWD(key), expected_count, FWD(callback));
			ret &= add_key_map_entries(FWD(key_map), FWD(args)...);
			return ret;
		}

		template<derived_ordered_map Map>
		NodeCallback auto expect_dictionary_key_map_and_length_and_default(
			Map&& key_map, LengthCallback auto&& length_callback, MapKeyValueCallback<Map> auto&& default_callback
		) {
			return [length_callback = FWD(length_callback), default_callback = FWD(default_callback), key_map = MOV(key_map)](
				ovdl::v2script::ast::Node const* node
			) mutable -> bool {
				bool ret = expect_dictionary_and_length(
					FWD(length_callback), dictionary_keys_callback(key_map, FWD(default_callback))
				)(node);
				ret &= check_key_map_counts(key_map);
				return ret;
			};
		}

		template<derived_ordered_map Map>
		NodeCallback auto expect_dictionary_key_map_and_length(
			Map&& key_map, LengthCallback auto&& length_callback
		) {
			return expect_dictionary_key_map_and_length_and_default(
				FWD(key_map), FWD(length_callback), map_key_value_invalid_callback<Map>
			);
		}

		template<derived_ordered_map Map>
		NodeCallback auto expect_dictionary_key_map_and_default(
			Map&& key_map, MapKeyValueCallback<Map> auto&& default_callback
		) {
			return expect_dictionary_key_map_and_length_and_default(
				FWD(key_map), default_length_callback, FWD(default_callback)
			);
		}

		template<string_map_case Case>
		NodeCallback auto expect_dictionary_key_map(template_key_map_t<Case>&& key_map) {
			return expect_dictionary_key_map_and_length_and_default(
				MOV(key_map), default_length_callback, map_key_value_ignore_invalid_callback<template_key_map_t<Case>> // we use map_key_value_ignore_invalid_callback here as some mods add extraneous keys (like maxWidth) which V2 ignores, so we must too
			);
		}

		template<derived_ordered_map Map, typename... Args>
		NodeCallback auto expect_dictionary_key_map_and_length_and_default(
			Map&& key_map, LengthCallback auto&& length_callback, MapKeyValueCallback<Map> auto&& default_callback,
			Args&&... args
		) {
			// TODO - pass return value back up (part of big key_map_t rewrite?)
			add_key_map_entries(FWD(key_map), FWD(args)...);
			return expect_dictionary_key_map_and_length_and_default(FWD(key_map), FWD(length_callback), FWD(default_callback));
		}

		template<derived_ordered_map Map, typename... Args>
		NodeCallback auto expect_dictionary_key_map_and_length(
			Map&& key_map, LengthCallback auto&& length_callback, Args&&... args
		) {
			add_key_map_entries(FWD(key_map), FWD(args)...);
			return expect_dictionary_key_map_and_length(FWD(key_map), FWD(length_callback));
		}

		template<derived_ordered_map Map, typename... Args>
		NodeCallback auto expect_dictionary_key_map_and_default_map(
			Map&& key_map, MapKeyValueCallback<Map> auto&& default_callback, Args&&... args
		) {
			add_key_map_entries(FWD(key_map), FWD(args)...);
			return expect_dictionary_key_map_and_default(FWD(key_map), FWD(default_callback));
		}
		template<derived_ordered_map Map, typename... Args>
		NodeCallback auto expect_dictionary_key_map_and_default(
			Map&& key_map, KeyValueCallback auto&& default_callback, Args&&... args
		) {
			return expect_dictionary_key_map_and_default_map(
				key_map,
				ignore_map<Map>(FWD(default_callback)),
				FWD(args)...
			);
		}

		template<derived_ordered_map Map, typename... Args>
		NodeCallback auto expect_dictionary_key_map(Map&& key_map, Args&&... args) {
			add_key_map_entries(FWD(key_map), FWD(args)...);
			return expect_dictionary_key_map(FWD(key_map));
		}

		template<string_map_case Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_and_length_and_default(
			LengthCallback auto&& length_callback, MapKeyValueCallback<template_key_map_t<Case>> auto&& default_callback, Args&&... args
		) {
			return expect_dictionary_key_map_and_length_and_default(
				template_key_map_t<Case> {}, FWD(length_callback), FWD(default_callback), FWD(args)...
			);
		}

		template<string_map_case Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_and_length(LengthCallback auto&& length_callback, Args&&... args) {
			return expect_dictionary_key_map_and_length_and_default(
				template_key_map_t<Case> {}, FWD(length_callback), map_key_value_invalid_callback<template_key_map_t<Case>>, FWD(args)...
			);
		}

		template<string_map_case Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_and_default_map(MapKeyValueCallback<template_key_map_t<Case>> auto&& default_callback, Args&&... args) {
			return expect_dictionary_key_map_and_length_and_default(
				template_key_map_t<Case> {}, default_length_callback, FWD(default_callback), FWD(args)...
			);
		}
		template<string_map_case Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_and_default(KeyValueCallback auto&& default_callback, Args&&... args) {
			return expect_dictionary_keys_and_default_map(
				ignore_map<template_key_map_t<Case>>(FWD(default_callback)),
				FWD(args)...
			);
		}

		template<string_map_case Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys(Args&&... args) {
			return expect_dictionary_key_map_and_length_and_default(
				template_key_map_t<Case> {}, default_length_callback, map_key_value_invalid_callback<template_key_map_t<Case>>, FWD(args)...
			);
		}

		LengthCallback auto reserve_length_callback(reservable auto& reservable) {
			return [&reservable](size_t size) -> size_t {
				reserve_more(reservable, size);
				return size;
			};
		}
		NodeCallback auto expect_list_reserve_length(reservable auto& reservable, NodeCallback auto&& callback) {
			return expect_list_and_length(reserve_length_callback(reservable), FWD(callback));
		}
		NodeCallback auto expect_dictionary_reserve_length(reservable auto& reservable, KeyValueCallback auto&& callback) {
			return expect_dictionary_and_length(reserve_length_callback(reservable), FWD(callback));
		}
		template<derived_ordered_map Map, typename... Args>
		NodeCallback auto expect_dictionary_key_map_reserve_length_and_default(
			reservable auto& reservable, Map&& key_map, KeyValueCallback auto&& default_callback,
			Args&&... args
		) {
			return expect_dictionary_key_map_and_length_and_default(
				FWD(key_map), reserve_length_callback(reservable), FWD(default_callback), FWD(args)...
			);
		}
		template<derived_ordered_map Map, typename... Args>
		NodeCallback auto expect_dictionary_key_map_reserve_length(
			reservable auto& reservable, Map&& key_map, Args&&... args
		) {
			return expect_dictionary_key_map_and_length(FWD(key_map), reserve_length_callback(reservable), FWD(args)...);
		}

		template<string_map_case Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_reserve_length_and_default_map(
			reservable auto& reservable, MapKeyValueCallback<template_key_map_t<Case>> auto&& default_callback, Args&&... args
		) {
			return expect_dictionary_keys_and_length_and_default<Case>(
				reserve_length_callback(reservable), FWD(default_callback), FWD(args)...
			);
		}
		template<string_map_case Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_reserve_length_and_default(
			reservable auto& reservable, KeyValueCallback auto&& default_callback, Args&&... args
		) {
			return expect_dictionary_keys_reserve_length_and_default_map<Case>(
				reservable,
				ignore_map<template_key_map_t<Case>>(FWD(default_callback)),
				FWD(args)...
			);
		}

		template<string_map_case Case = StringMapCaseSensitive, typename... Args>
		NodeCallback auto expect_dictionary_keys_reserve_length(reservable auto& reservable, Args&&... args) {
			return expect_dictionary_keys_and_length<Case>(reserve_length_callback(reservable), FWD(args)...);
		}

		NodeTools::node_callback_t name_list_callback(NodeTools::callback_t<memory::vector<memory::string>&&> callback);

		template<typename T, string_map_case Case>
		Callback<std::string_view> auto expect_mapped_string(
			template_string_map_t<T, Case> const& map, Callback<T> auto&& callback, bool warn = false
		) {
			return [&map, callback = FWD(callback), warn](std::string_view string) mutable -> bool {
				const typename template_string_map_t<T, Case>::const_iterator it = map.find(string);
				if (it != map.end()) {
					return callback(it->second);
				}
				spdlog::log_s(
					warn ? spdlog::level::warn : spdlog::level::err,
					"\"{}\" is not a valid key. Valid keys: [{}]",
					string, string_join(map)
				);
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
		NodeTools::callback_t<T> assign_variable_callback_cast(auto& var) {
			return [&var](T val) -> bool {
				var = val;
				return true;
			};
		}

		template<typename T>
		Callback<T> auto assign_variable_callback(T& var) {
			return assign_variable_callback_cast<T, T>(var);
		}

		/* By default this will only allow an optional to be set once. Set allow_overwrite
		 * to true to allow multiple assignments, with the last taking precedence. */
		template<typename T>
		Callback<T> auto assign_variable_callback_opt(
			std::optional<T>& var, bool allow_overwrite = false
		) {
			return [&var, allow_overwrite](T const& val) -> bool {
				if (!allow_overwrite && var.has_value()) {
					spdlog::error_s("Cannot assign value to already-initialised optional!");
					return false;
				}
				var = val;
				return true;
			};
		}

		/* By default this will only allow an optional to be set once. Set allow_overwrite
		 * to true to allow multiple assignments, with the last taking precedence. */
		template<typename T>
		auto emplace_opt_callback(
			std::optional<T>& var, bool allow_overwrite = false
		) {
			return [&var, allow_overwrite](auto&&... args) -> bool {
				if (!allow_overwrite && var.has_value()) {
					spdlog::error_s("Cannot assign value to already-initialised optional!");
					return false;
				}
				var.emplace(FWD(args)...);
				return true;
			};
		}

		NodeTools::callback_t<std::string_view> assign_variable_callback_string(memory::string& var);

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
			return [&var](std::string_view, ovdl::v2script::ast::Node const*) -> bool {
				var++;
				return true;
			};
		}

		template<typename T>
		Callback<T&> auto assign_variable_callback_pointer(T*& var) {
			return [&var](T& val) -> bool {
				var = &val;
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
					spdlog::error_s("Cannot assign pointer value to already-initialised optional!");
					return false;
				}
				var = &val;
				return true;
			};
		}

		template<typename T, typename U>
		Callback<T> auto vector_callback(memory::vector<U>& vec) {
			return [&vec](T val) -> bool {
				vec.emplace_back(std::move(val));
				return true;
			};
		}

		template<typename T>
		Callback<T> auto vector_callback(memory::vector<T>& vec) {
			return vector_callback<T, T>(vec);
		}

		template<typename T>
		auto vector_emplace_callback(memory::vector<T>& vec) {
			return [&vec](auto&&... args) -> bool {
				vec.emplace_back(FWD(args)...);
				return true;
			};
		}

		template<typename T>
		Callback<T&> auto vector_callback_pointer(memory::vector<T*>& vec) {
			return [&vec](T& val) -> bool {
				vec.emplace_back(&val);
				return true;
			};
		}

		NodeTools::callback_t<std::string_view> vector_callback_string(memory::vector<memory::string>& vec);

		template<typename T, typename U, typename... SetArgs>
		Callback<T> auto set_callback(tsl::ordered_set<U, SetArgs...>& set, bool warn = false) {
			return [&set, warn](T val) -> bool {
				if (set.emplace(std::move(val)).second) {
					return true;
				}
				spdlog::log_s(
					warn ? spdlog::level::warn : spdlog::level::err,
					"Duplicate set entry: \"{}\"", val
				);
				return warn;
			};
		}

		template<typename T, typename... SetArgs>
		Callback<T const&> auto set_callback_pointer(tsl::ordered_set<T const*, SetArgs...>& set, bool warn = false) {
			return [&set, warn](T const& val) -> bool {
				if (set.emplace(&val).second) {
					return true;
				}
				spdlog::log_s(
					warn ? spdlog::level::warn : spdlog::level::err,
					"Duplicate set entry: \"{}\"",
					val
				);
				return warn;
			};
		}

		template<typename Key, typename Value, typename... MapArgs>
		Callback<Value> auto map_callback(
			tsl::ordered_map<Key, Value, MapArgs...>& map, Key key, bool warn = false
		) {
			return [&map, key, warn](Value value) -> bool {
				if (map.emplace(key, std::move(value)).second) {
					return true;
				}
				if constexpr(std::is_pointer_v<Key>) {
					spdlog::log_s(
						warn ? spdlog::level::warn : spdlog::level::err,
						"Duplicate map entry with key: \"{}\"", ovfmt::validate(key)
					);
				} else {
					spdlog::log_s(
						warn ? spdlog::level::warn : spdlog::level::err,
						"Duplicate map entry with key: \"{}\"", key
					);
				}
				return warn;
			};
		}

		template<typename Key, typename Value>
		Callback<Value> auto map_callback(
			IndexedFlatMap<Key, Value>& map, Key const* key, bool warn = false
		) {
			return [&map, key, warn](Value value) -> bool {
				if (key == nullptr) {
					spdlog::error_s("Null key in map_callback");
					return false;
				}
				Value& map_value = map.at(*key);
				bool ret = true;
				if (map_value != Value {}) {
					spdlog::log_s(warn ? spdlog::level::warn : spdlog::level::err, "Duplicate map entry with key: \"{}\"", *key);
					ret = warn;
				}
				map_value = std::move(value);
				return ret;
			};
		}

		/* Often used for rotations which must be negated due to OpenVic's coordinate system being orientated
		 * oppositely to Vic2's. */
		template<typename T = fixed_point_t>
		constexpr Callback<T> auto negate_callback(Callback<T> auto&& callback) {
			return [callback](T val) -> bool {
				return callback(-val);
			};
		}

		/* Often used for map-space coordinates which must have their y-coordinate flipped due to OpenVic using the
		 * top-left of the map as the origin as opposed Vic2 using the bottom-left. */
		template<typename T>
		constexpr Callback<vec2_t<T>> auto flip_y_callback(Callback<vec2_t<T>> auto&& callback, T height) {
			return [callback, height](vec2_t<T> val) -> bool {
				val.y = height - val.y;
				return callback(val);
			};
		}

		template<typename T>
		constexpr Callback<T> auto flip_height_callback(Callback<T> auto&& callback, T height) {
			return [callback, height](T val) -> bool {
				return callback(height - val);
			};
		}
	}
}

#undef FWD
#undef MOV
