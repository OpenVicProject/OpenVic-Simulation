#include "NodeTools.hpp"

#include <charconv>
#include <concepts>
#include <string>
#include <string_view>
#include <system_error>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/detail/Utility.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

#include <range/v3/iterator/operations.hpp>
#include <range/v3/view/enumerate.hpp>

#include "openvic-simulation/core/template/FunctionalConcepts.hpp"
#include "openvic-simulation/core/ui/TextFormat.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/fixed_point/String.hpp"
#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/utility/Getters.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

#define MOV(...) static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define FWD(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

template<typename T>
static NodeCallback auto _expect_type(Callback<T const*> auto&& callback) {
	return [callback = FWD(callback)](ovdl::v2script::ast::Node const* node) mutable -> bool {
		if (node != nullptr) {
			T const* cast_node = dryad::node_try_cast<T>(node);
			if (cast_node != nullptr) {
				return callback(cast_node);
			}
			spdlog::error_s(
				"Invalid node type {} when expecting {}",
				ast::get_type_name(node->kind()), type_name<T>()
			);
		} else {
			spdlog::error_s("Null node when expecting {}", type_name<T>());
		}
		return false;
	};
}

using _NodeIterator = typename decltype(std::declval<ovdl::v2script::ast::Node const*>()->children())::iterator;
using _NodeStatementRange = dryad::node_range<_NodeIterator, ovdl::v2script::ast::Statement>;

static NodeCallback auto _abstract_statement_node_callback(Callback<_NodeStatementRange> auto&& callback) {
	return [callback = FWD(callback)](ovdl::v2script::ast::Node const* node) mutable -> bool {
		if (node != nullptr) {
			if (auto const* file_tree = dryad::node_try_cast<ovdl::v2script::ast::FileTree>(node)) {
				return callback(file_tree->statements());
			}
			if (auto const* list_value = dryad::node_try_cast<ovdl::v2script::ast::ListValue>(node)) {
				return callback(list_value->statements());
			}
			spdlog::error_s(
				"Invalid node type {} when expecting {} or {}",
				ast::get_type_name(node->kind()), type_name<ovdl::v2script::ast::FileTree>(), type_name<ovdl::v2script::ast::ListValue>()
			);
		} else {
			spdlog::error_s(
				"Null node when expecting {} or {}",
				type_name<ovdl::v2script::ast::FileTree>(), type_name<ovdl::v2script::ast::ListValue>()
			);
		}
		return false;
	};
}

template<std::derived_from<ovdl::v2script::ast::FlatValue> T>
static Callback<T const*> auto _abstract_symbol_node_callback(Callback<ovdl::symbol<char>> auto&& callback, bool allow_empty) {
	return [callback = FWD(callback), allow_empty](T const* node) mutable -> bool {
		if (allow_empty) {
			return callback(node->value());
		} else {
			if (node->value()) {
				return callback(node->value());
			} else {
				spdlog::error_s("Invalid string value - empty!");
				return false;
			}
		}
	};
}

template<std::derived_from<ovdl::v2script::ast::FlatValue> T>
static Callback<T const*> auto _abstract_string_node_callback(Callback<std::string_view> auto&& callback, bool allow_empty) {
	return _abstract_symbol_node_callback<T>(
		[callback = FWD(callback)](ovdl::symbol<char> symbol) mutable -> bool {
			return callback(symbol.view());
		},
		allow_empty
	);
}

NodeTools::node_callback_t NodeTools::expect_identifier(NodeTools::callback_t<std::string_view> callback) {
	return _expect_type<ovdl::v2script::ast::IdentifierValue>(_abstract_string_node_callback<ovdl::v2script::ast::IdentifierValue>(callback, false));
}

static NodeCallback auto _expect_identifier(Callback<ovdl::symbol<char>> auto&& callback) {
	return _expect_type<ovdl::v2script::ast::IdentifierValue>(_abstract_symbol_node_callback<ovdl::v2script::ast::IdentifierValue>(FWD(callback), false));
}

NodeTools::node_callback_t NodeTools::expect_string(NodeTools::callback_t<std::string_view> callback, bool allow_empty) {
	return _expect_type<ovdl::v2script::ast::StringValue>(_abstract_string_node_callback<ovdl::v2script::ast::StringValue>(callback, allow_empty));
}

static NodeCallback auto _expect_string(Callback<ovdl::symbol<char>> auto&& callback, bool allow_empty) {
	return _expect_type<ovdl::v2script::ast::StringValue>(_abstract_symbol_node_callback<ovdl::v2script::ast::StringValue>(FWD(callback), allow_empty));
}

NodeTools::node_callback_t NodeTools::expect_identifier_or_string(NodeTools::callback_t<std::string_view> callback, bool allow_empty) {
	return [callback, allow_empty](ovdl::v2script::ast::Node const* node) -> bool {
		if (node != nullptr) {
			auto const* cast_node = dryad::node_try_cast<ovdl::v2script::ast::FlatValue>(node);
			if (cast_node != nullptr) {
				return _abstract_string_node_callback<ovdl::v2script::ast::FlatValue>(FWD(callback), allow_empty)(cast_node);
			}
			spdlog::error_s(
				"Invalid node type {} when expecting {} or {}",
				ast::get_type_name(node->kind()), type_name<ovdl::v2script::ast::IdentifierValue>(), type_name<ovdl::v2script::ast::StringValue>()
			);
		} else {
			spdlog::error_s(
				"Null node when expecting {} or {}",
				type_name<ovdl::v2script::ast::IdentifierValue>(), type_name<ovdl::v2script::ast::StringValue>()
			);
		}
		return false;
	};
}

NodeTools::node_callback_t NodeTools::expect_bool(NodeTools::callback_t<bool> callback) {
	static const case_insensitive_string_map_t<bool> bool_map { { "yes", true }, { "no", false } };
	return expect_identifier(expect_mapped_string(bool_map, callback));
}

NodeTools::node_callback_t NodeTools::expect_int_bool(NodeTools::callback_t<bool> callback) {
	return expect_uint64([callback](uint64_t num) mutable -> bool {
		if (num > 1) {
			spdlog::warn_s("Found int bool with value >1: {}", num);
		}
		return callback(num != 0);
	});
}

NodeTools::node_callback_t NodeTools::expect_int64(NodeTools::callback_t<int64_t> callback, int base) {
	return expect_identifier([callback, base](std::string_view identifier) mutable -> bool {
		int64_t val;
		std::from_chars_result result = string_to_int64(identifier, val, base);
		if (result.ec == std::errc{}) {
			return callback(val);
		}
		spdlog::error_s("Invalid int identifier text: {}", identifier);
		return false;
	});
}

NodeTools::node_callback_t NodeTools::expect_uint64(NodeTools::callback_t<uint64_t> callback, int base) {
	return expect_identifier([callback, base](std::string_view identifier) mutable -> bool {
		uint64_t val;
		std::from_chars_result result = string_to_uint64(identifier, val, base);
		if (result.ec == std::errc{}) {
			return callback(val);
		}
		spdlog::error_s("Invalid uint identifier text: {}", identifier);
		return false;
	});
}

NodeTools::callback_t<std::string_view> NodeTools::expect_fixed_point_str(NodeTools::callback_t<fixed_point_t> callback) {
	return [callback](std::string_view identifier) mutable -> bool {
		bool successful = false;
		fixed_point_t val;
		const std::from_chars_result result = fp::from_chars_with_plus(val, identifier.data(), identifier.data()+identifier.length());
		if (result.ec == std::errc{}) {
			return callback(val);
		}
		spdlog::error_s("Invalid fixed point identifier text: {}", identifier);
		return false;
	};
}

NodeTools::node_callback_t NodeTools::expect_fixed_point(NodeTools::callback_t<fixed_point_t> callback) {
	return expect_identifier(expect_fixed_point_str(callback));
}

NodeTools::node_callback_t NodeTools::expect_colour(NodeTools::callback_t<colour_t> callback) {
	return [callback](ovdl::v2script::ast::Node const* node) mutable -> bool {
		colour_t col = colour_t::null();
		int32_t components = 0;
		bool ret = expect_list_of_length(3, expect_fixed_point(
			[&col, &components](fixed_point_t val) -> bool {
				if (val < 0 || val > 255) {
					spdlog::error_s(
						"Invalid colour component #{}: {}",
						components++, val
					);
					return false;
				} else {
					if (val <= 1) {
						val *= 255;
					} else if (!val.is_integer()) {
						spdlog::warn_s("Fractional part of colour component #{} will be truncated: {}", components, val);
					}
					col[components++] = val.truncate<colour_t::value_type>();
					return true;
				}
			}
		))(node);
		if (ret) {
			ret &= callback(col);
		}
		return ret;
	};
}

NodeTools::node_callback_t NodeTools::expect_colour_hex(NodeTools::callback_t<colour_argb_t> callback) {
	return expect_uint<colour_argb_t::integer_type>([callback](colour_argb_t::integer_type val) mutable -> bool {
		return callback(colour_argb_t::from_argb(val));
	}, 16);
}

NodeTools::callback_t<std::string_view> NodeTools::expect_date_str(NodeTools::callback_t<Date> callback) {
	return [callback](std::string_view identifier) mutable -> bool {
		Date::from_chars_result result;
		const Date date = Date::from_string_log(identifier, &result);
		if (result.ec == std::errc{}) {
			return callback(date);
		}
		spdlog::error_s("Invalid date identifier text: {}", identifier);
		return false;
	};
}

NodeTools::node_callback_t NodeTools::expect_date(NodeTools::callback_t<Date> callback) {
	return expect_identifier(expect_date_str(callback));
}

NodeTools::node_callback_t NodeTools::expect_date_string(NodeTools::callback_t<Date> callback) {
	return expect_string(expect_date_str(callback));
}

NodeTools::node_callback_t NodeTools::expect_date_identifier_or_string(NodeTools::callback_t<Date> callback) {
	return expect_identifier_or_string(expect_date_str(callback));
}

NodeTools::node_callback_t NodeTools::expect_years(NodeTools::callback_t<Timespan> callback) {
	return expect_int<Timespan::day_t>([callback](Timespan::day_t val) mutable -> bool {
		return callback(Timespan::from_years(val));
	});
}

NodeTools::node_callback_t NodeTools::expect_months(NodeTools::callback_t<Timespan> callback) {
	return expect_int<Timespan::day_t>([callback](Timespan::day_t val) mutable -> bool {
		return callback(Timespan::from_months(val));
	});
}

NodeTools::node_callback_t NodeTools::expect_days(NodeTools::callback_t<Timespan> callback) {
	return expect_int<Timespan::day_t>([callback](Timespan::day_t val) mutable -> bool {
		return callback(Timespan::from_days(val));
	});
}

template<typename T, NodeTools::node_callback_t (*expect_func)(NodeTools::callback_t<T>)>
NodeCallback auto _expect_vec2(Callback<vec2_t<T>> auto&& callback) {
	return [callback = FWD(callback)](ovdl::v2script::ast::Node const* node) mutable -> bool {
		vec2_t<T> vec;
		bool ret = expect_dictionary_keys(
			"x", ONE_EXACTLY, expect_func(assign_variable_callback(vec.x)),
			"y", ONE_EXACTLY, expect_func(assign_variable_callback(vec.y))
		)(node);
		if (ret) {
			ret &= callback(vec);
		}
		return ret;
	};
}

NodeTools::node_callback_t NodeTools::expect_text_format(NodeTools::callback_t<text_format_t> callback) {
	using enum text_format_t;

	static const string_map_t<text_format_t> format_map = {
		{ "left",  left },
		{ "right", right },
		{ "centre", centre },
		{ "center", centre },
		{ "justified", justified }
	};

	return expect_identifier(expect_mapped_string(format_map, callback));
}

static NodeTools::node_callback_t _expect_int(NodeTools::callback_t<ivec2_t::type> callback) {
	return expect_int(callback);
}

NodeTools::node_callback_t NodeTools::expect_ivec2(NodeTools::callback_t<ivec2_t> callback) {
	return _expect_vec2<ivec2_t::type, _expect_int>(callback);
}

NodeTools::node_callback_t NodeTools::expect_fvec2(NodeTools::callback_t<fvec2_t> callback) {
	return _expect_vec2<fixed_point_t, expect_fixed_point>(callback);
}

// seen in some gfx files, these vectors don't have x,y,z,w labels, so are loaded similarly to colours.

NodeTools::node_callback_t NodeTools::expect_fvec3(NodeTools::callback_t<fvec3_t> callback) {
	return [callback](ovdl::v2script::ast::Node const* node) mutable -> bool {
		fvec3_t vec;
		int32_t components = 0;
		bool ret = expect_list_of_length(3, expect_fixed_point(
			[&vec, &components](fixed_point_t val) -> bool {
				vec[components++] = val;
				return true;
			}
		))(node);
		if (ret) {
			ret &= callback(vec);
		}
		return ret;
	};
}

NodeTools::node_callback_t NodeTools::expect_fvec4(NodeTools::callback_t<fvec4_t> callback) {
	return [callback](ovdl::v2script::ast::Node const* node) mutable -> bool {
		fvec4_t vec;
		int32_t components = 0;
		bool ret = expect_list_of_length(4, expect_fixed_point(
			[&vec, &components](fixed_point_t val) -> bool {
				vec[components++] = val;
				return true;
			}
		))(node);
		if (ret) {
			ret &= callback(vec);
		}
		return ret;
	};
}

NodeTools::node_callback_t NodeTools::expect_assign(key_value_callback_t callback) {
	return _expect_type<ovdl::v2script::ast::AssignStatement>([callback](ovdl::v2script::ast::AssignStatement const* assign_node) mutable -> bool {
		std::string_view left;
		bool ret = expect_identifier(assign_variable_callback(left))(assign_node->left());
		if (ret) {
			ret &= callback(left, assign_node->right());
			if (!ret) {
				spdlog::error_s("Callback failed for assign node with key: {}", left);
			}
		} else {
			spdlog::error_s("Callback key failed for assign node with key: {}", left);
		}
		return ret;
	});
}

NodeTools::node_callback_t NodeTools::expect_list_and_length(length_callback_t length_callback, NodeTools::node_callback_t callback) {
	return _abstract_statement_node_callback([length_callback, callback](_NodeStatementRange list) mutable -> bool {
		bool ret = true;
		size_t dist = ranges::distance(list);
		size_t size = length_callback(dist);

		if (size > dist) {
			spdlog::error_s(
				"Trying to read more values than the list contains: {} > {}",
				size, dist
			);
			size = dist;
			ret = false;
		}
		for (auto [index, sub_node] : list | ranges::views::enumerate) {
			if (index >= size) {
				return ret;
			}
			if (auto const* value = dryad::node_try_cast<ovdl::v2script::ast::ValueStatement>(sub_node)) {
				ret &= callback(value->value());
				continue;
			}
			ret &= callback(sub_node);
		}
		return ret;
	});
}

NodeTools::node_callback_t NodeTools::expect_list_of_length(size_t length, NodeTools::node_callback_t callback) {
	return [length, callback](ovdl::v2script::ast::Node const* node) -> bool {
		bool size_ret = true;
		bool ret = expect_list_and_length(
			[length, &size_ret](size_t size) -> size_t {
				if (size != length) {
					spdlog::error_s(
						"List length {} does not match expected length {}",
						size, length
					);
					size_ret = false;
					if (length < size) {
						return length;
					}
				}
				return size;
			},
			callback
		)(node);
		return ret && size_ret;
	};
}

NodeTools::node_callback_t NodeTools::expect_list(NodeTools::node_callback_t callback) {
	return expect_list_and_length(default_length_callback, callback);
}

NodeTools::node_callback_t NodeTools::expect_length(NodeTools::callback_t<size_t> callback) {
	return [callback](ovdl::v2script::ast::Node const* node) mutable -> bool {
		bool size_ret = true;
		bool ret = expect_list_and_length(
			[callback, &size_ret](size_t size) mutable -> size_t {
				size_ret &= callback(size);
				return 0;
			},
			success_callback
		)(node);
		return ret && size_ret;
	};
}

template<typename Key>
static NodeTools::node_callback_t _expect_key(Key key, NodeCallback auto&& callback, bool* key_found, bool allow_duplicates) {
	if constexpr (std::same_as<Key, ovdl::symbol<char>>) {
		if (!key) {
			if (key_found != nullptr) {
				*key_found = false;
			}
			return [](ovdl::v2script::ast::Node const*) -> bool {
				spdlog::error_s("Failed to find expected interned key.");
				return false;
			};
		}
	}

	static constexpr auto assign_left = [](Key& left) {
		if constexpr (std::same_as<Key, std::string_view>) {
			return expect_identifier(assign_variable_callback(left));
		} else if (std::same_as<Key, ovdl::symbol<char>>) {
			return _expect_identifier(assign_variable_callback_cast<ovdl::symbol<char>>(left));
		}
	};

	return _abstract_statement_node_callback([key, callback = FWD(callback), key_found, allow_duplicates](_NodeStatementRange list) mutable -> bool {
		bool ret = true;
		size_t keys_found = 0;
		for (ovdl::v2script::ast::Statement const* sub_node : list) {
			auto const* assign_node = dryad::node_try_cast<ovdl::v2script::ast::AssignStatement>(sub_node);
			if (assign_node == nullptr) {
				continue;
			}
			Key left;
			if (!assign_left(left)(assign_node->left())) {
				continue;
			}
			if (left == key) {
				if (keys_found++ == 0) {
					ret &= callback(assign_node->right());
					if (allow_duplicates) {
						break;
					}
				}
			}
		}
		std::string_view key_str = [&] {
			if constexpr (std::same_as<Key, std::string_view>) {
				return key;
			} else {
				return key.view();
			}
		}();
		if (keys_found == 0) {
			if (key_found != nullptr) {
				*key_found = false;
			} else {
				spdlog::error_s("Failed to find expected key: \"{}\"", key_str);
			}
			ret = false;
		} else {
			if (key_found != nullptr) {
				*key_found = true;
			}
			if (!allow_duplicates && keys_found > 1) {
				spdlog::error_s(
					"Found {} instances of key: \"{}\" (expected 1)",
					keys_found, key_str
				);
				ret = false;
			}
		}
		return ret;
	});
}

NodeTools::node_callback_t NodeTools::expect_key(std::string_view key, NodeTools::node_callback_t callback, bool* key_found, bool allow_duplicates) {
	return _expect_key(key, callback, key_found, allow_duplicates);
}

NodeTools::node_callback_t
NodeTools::expect_key(ovdl::symbol<char> key, NodeTools::node_callback_t callback, bool* key_found, bool allow_duplicates) {
	return _expect_key(key, callback, key_found, allow_duplicates);
}

NodeTools::node_callback_t NodeTools::expect_dictionary_and_length(length_callback_t length_callback, key_value_callback_t callback) {
	return expect_list_and_length(length_callback, expect_assign(callback));
}

NodeTools::node_callback_t NodeTools::expect_dictionary(key_value_callback_t callback) {
	return expect_dictionary_and_length(default_length_callback, callback);
}

NodeTools::node_callback_t NodeTools::name_list_callback(NodeTools::callback_t<name_list_t&&> callback) {
	return [callback](ovdl::v2script::ast::Node const* node) mutable -> bool {
		name_list_t list;
		bool ret = expect_list_reserve_length(
			list, expect_identifier_or_string(vector_callback<std::string_view>(list))
		)(node);
		ret &= callback(std::move(list));
		return ret;
	};
}

std::ostream& OpenVic::operator<<(std::ostream& stream, name_list_t const& name_list) {
	stream << '[';
	if (!name_list.empty()) {
		stream << name_list.front();
		std::for_each(name_list.begin() + 1, name_list.end(), [&stream](memory::string const& name) -> void {
			stream << ", " << name;
		});
	}
	return stream << ']';
}

NodeTools::callback_t<std::string_view> NodeTools::assign_variable_callback_string(memory::string& var) {
	return assign_variable_callback_cast<std::string_view>(var);
}

NodeTools::callback_t<std::string_view> NodeTools::vector_callback_string(memory::vector<memory::string>& vec) {
	return [&vec](std::string_view val) -> bool {
		vec.emplace_back(memory::string(val));
		return true;
	};
}
