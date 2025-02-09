#include "NodeTools.hpp"

#include <concepts>
#include <string_view>
#include <system_error>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/detail/Utility.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

#include <range/v3/iterator/operations.hpp>
#include <range/v3/view/enumerate.hpp>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/TextFormat.hpp"
#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/utility/Getters.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

#define MOV(...) static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define FWD(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

template<typename T>
static NodeCallback auto _expect_type(Callback<T const*> auto&& callback) {
	return [callback = FWD(callback)](ast::NodeCPtr node) -> bool {
		if (node != nullptr) {
			T const* cast_node = dryad::node_try_cast<T>(node);
			if (cast_node != nullptr) {
				return callback(cast_node);
			}
			Logger::error("Invalid node type ", ast::get_type_name(node->kind()), " when expecting ", utility::type_name<T>());
		} else {
			Logger::error("Null node when expecting ", utility::type_name<T>());
		}
		return false;
	};
}

using _NodeIterator = typename decltype(std::declval<ast::NodeCPtr>()->children())::iterator;
using _NodeStatementRange = dryad::node_range<_NodeIterator, ast::Statement>;

static NodeCallback auto _abstract_statement_node_callback(Callback<_NodeStatementRange> auto&& callback) {
	return [callback = FWD(callback)](ast::NodeCPtr node) -> bool {
		if (node != nullptr) {
			if (auto const* file_tree = dryad::node_try_cast<ast::FileTree>(node)) {
				return callback(file_tree->statements());
			}
			if (auto const* list_value = dryad::node_try_cast<ast::ListValue>(node)) {
				return callback(list_value->statements());
			}
			Logger::error(
				"Invalid node type ", ast::get_type_name(node->kind()), " when expecting ", utility::type_name<ast::FileTree>(), " or ",
				utility::type_name<ast::ListValue>()
			);
		} else {
			Logger::error(
				"Null node when expecting ", utility::type_name<ast::FileTree>(), " or ", utility::type_name<ast::ListValue>()
			);
		}
		return false;
	};
}

template<std::derived_from<ast::FlatValue> T>
static Callback<T const*> auto _abstract_symbol_node_callback(Callback<ovdl::symbol<char>> auto&& callback, bool allow_empty) {
	return [callback = FWD(callback), allow_empty](T const* node) -> bool {
		if (allow_empty) {
			return callback(node->value());
		} else {
			if (node->value()) {
				return callback(node->value());
			} else {
				Logger::error("Invalid string value - empty!");
				return false;
			}
		}
	};
}

template<std::derived_from<ast::FlatValue> T>
static Callback<T const*> auto _abstract_string_node_callback(Callback<std::string_view> auto callback, bool allow_empty) {
	return _abstract_symbol_node_callback<T>(
		[callback](ovdl::symbol<char> symbol) -> bool {
			return callback(symbol.view());
		},
		allow_empty
	);
}

node_callback_t NodeTools::expect_identifier(callback_t<std::string_view> callback) {
	return _expect_type<ast::IdentifierValue>(_abstract_string_node_callback<ast::IdentifierValue>(callback, false));
}

static NodeCallback auto _expect_identifier(Callback<ovdl::symbol<char>> auto callback) {
	return _expect_type<ast::IdentifierValue>(_abstract_symbol_node_callback<ast::IdentifierValue>(callback, false));
}

node_callback_t NodeTools::expect_string(callback_t<std::string_view> callback, bool allow_empty) {
	return _expect_type<ast::StringValue>(_abstract_string_node_callback<ast::StringValue>(callback, allow_empty));
}

static NodeCallback auto _expect_string(Callback<ovdl::symbol<char>> auto callback, bool allow_empty) {
	return _expect_type<ast::StringValue>(_abstract_symbol_node_callback<ast::StringValue>(callback, allow_empty));
}

node_callback_t NodeTools::expect_identifier_or_string(callback_t<std::string_view> callback, bool allow_empty) {
	return [callback, allow_empty](ast::NodeCPtr node) -> bool {
		if (node != nullptr) {
			auto const* cast_node = dryad::node_try_cast<ast::FlatValue>(node);
			if (cast_node != nullptr) {
				return _abstract_string_node_callback<ast::FlatValue>(callback, allow_empty)(cast_node);
			}
			Logger::error(
				"Invalid node type ", ast::get_type_name(node->kind()), " when expecting ", utility::type_name<ast::IdentifierValue>(), " or ",
				utility::type_name<ast::StringValue>()
			);
		} else {
			Logger::error(
				"Null node when expecting ", utility::type_name<ast::IdentifierValue>(), " or ", utility::type_name<ast::StringValue>()
			);
		}
		return false;
	};
}

node_callback_t NodeTools::expect_bool(callback_t<bool> callback) {
	static const case_insensitive_string_map_t<bool> bool_map { { "yes", true }, { "no", false } };
	return expect_identifier(expect_mapped_string(bool_map, callback));
}

node_callback_t NodeTools::expect_int_bool(callback_t<bool> callback) {
	return expect_uint64([callback](uint64_t num) -> bool {
		if (num > 1) {
			Logger::warning("Found int bool with value >1: ", num);
		}
		return callback(num != 0);
	});
}

node_callback_t NodeTools::expect_int64(callback_t<int64_t> callback, int base) {
	return expect_identifier([callback, base](std::string_view identifier) -> bool {
		bool successful = false;
		const int64_t val = StringUtils::string_to_int64(identifier, &successful, base);
		if (successful) {
			return callback(val);
		}
		Logger::error("Invalid int identifier text: ", identifier);
		return false;
	});
}

node_callback_t NodeTools::expect_uint64(callback_t<uint64_t> callback, int base) {
	return expect_identifier([callback, base](std::string_view identifier) -> bool {
		bool successful = false;
		const uint64_t val = StringUtils::string_to_uint64(identifier, &successful, base);
		if (successful) {
			return callback(val);
		}
		Logger::error("Invalid uint identifier text: ", identifier);
		return false;
	});
}

callback_t<std::string_view> NodeTools::expect_fixed_point_str(callback_t<fixed_point_t> callback) {
	return [callback](std::string_view identifier) -> bool {
		bool successful = false;
		const fixed_point_t val = fixed_point_t::parse(identifier.data(), identifier.length(), &successful);
		if (successful) {
			return callback(val);
		}
		Logger::error("Invalid fixed point identifier text: ", identifier);
		return false;
	};
}

node_callback_t NodeTools::expect_fixed_point(callback_t<fixed_point_t> callback) {
	return expect_identifier(expect_fixed_point_str(callback));
}

node_callback_t NodeTools::expect_colour(callback_t<colour_t> callback) {
	return [callback](ast::NodeCPtr node) -> bool {
		colour_t col = colour_t::null();
		int32_t components = 0;
		bool ret = expect_list_of_length(3, expect_fixed_point(
			[&col, &components](fixed_point_t val) -> bool {
				if (val < 0 || val > 255) {
					Logger::error("Invalid colour component #", components++, ": ", val);
					return false;
				} else {
					if (val <= 1) {
						val *= 255;
					} else if (!val.is_integer()) {
						Logger::warning("Fractional part of colour component #", components, " will be truncated: ", val);
					}
					col[components++] = val.to_int64_t();
					return true;
				}
			}
		))(node);
		ret &= callback(col);
		return ret;
	};
}

node_callback_t NodeTools::expect_colour_hex(callback_t<colour_argb_t> callback) {
	return expect_uint<colour_argb_t::integer_type>([callback](colour_argb_t::integer_type val) -> bool {
		return callback(colour_argb_t::from_argb(val));
	}, 16);
}

callback_t<std::string_view> NodeTools::expect_date_str(callback_t<Date> callback) {
	return [callback](std::string_view identifier) -> bool {
		Date::from_chars_result result;
		const Date date = Date::from_string_log(identifier, &result);
		if (result.ec == std::errc{}) {
			return callback(date);
		}
		Logger::error("Invalid date identifier text: ", identifier);
		return false;
	};
}

node_callback_t NodeTools::expect_date(callback_t<Date> callback) {
	return expect_identifier(expect_date_str(callback));
}

node_callback_t NodeTools::expect_date_string(callback_t<Date> callback) {
	return expect_string(expect_date_str(callback));
}

node_callback_t NodeTools::expect_date_identifier_or_string(callback_t<Date> callback) {
	return expect_identifier_or_string(expect_date_str(callback));
}

node_callback_t NodeTools::expect_years(callback_t<Timespan> callback) {
	return expect_int<Timespan::day_t>([callback](Timespan::day_t val) -> bool {
		return callback(Timespan::from_years(val));
	});
}

node_callback_t NodeTools::expect_months(callback_t<Timespan> callback) {
	return expect_int<Timespan::day_t>([callback](Timespan::day_t val) -> bool {
		return callback(Timespan::from_months(val));
	});
}

node_callback_t NodeTools::expect_days(callback_t<Timespan> callback) {
	return expect_int<Timespan::day_t>([callback](Timespan::day_t val) -> bool {
		return callback(Timespan::from_days(val));
	});
}

template<typename T, node_callback_t (*expect_func)(callback_t<T>)>
NodeCallback auto _expect_vec2(Callback<vec2_t<T>> auto&& callback) {
	return [callback = FWD(callback)](ast::NodeCPtr node) -> bool {
		vec2_t<T> vec;
		bool ret = expect_dictionary_keys(
			"x", ONE_EXACTLY, expect_func(assign_variable_callback(vec.x)),
			"y", ONE_EXACTLY, expect_func(assign_variable_callback(vec.y))
		)(node);
		ret &= callback(vec);
		return ret;
	};
}

node_callback_t NodeTools::expect_text_format(callback_t<text_format_t> callback) {
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

static node_callback_t _expect_int(callback_t<ivec2_t::type> callback) {
	return expect_int(callback);
}

node_callback_t NodeTools::expect_ivec2(callback_t<ivec2_t> callback) {
	return _expect_vec2<ivec2_t::type, _expect_int>(callback);
}

node_callback_t NodeTools::expect_fvec2(callback_t<fvec2_t> callback) {
	return _expect_vec2<fixed_point_t, expect_fixed_point>(callback);
}

// seen in some gfx files, these vectors don't have x,y,z,w labels, so are loaded similarly to colours.

node_callback_t NodeTools::expect_fvec3(callback_t<fvec3_t> callback) {
	return [callback](ast::NodeCPtr node) -> bool {
		fvec3_t vec;
		int32_t components = 0;
		bool ret = expect_list_of_length(3, expect_fixed_point(
			[&vec, &components](fixed_point_t val) -> bool {
				vec[components++] = val;
				return true;
			}
		))(node);
		ret &= callback(vec);
		return ret;
	};
}

node_callback_t NodeTools::expect_fvec4(callback_t<fvec4_t> callback) {
	return [callback](ast::NodeCPtr node) -> bool {
		fvec4_t vec;
		int32_t components = 0;
		bool ret = expect_list_of_length(4, expect_fixed_point(
			[&vec, &components](fixed_point_t val) -> bool {
				vec[components++] = val;
				return true;
			}
		))(node);
		ret &= callback(vec);
		return ret;
	};
}

node_callback_t NodeTools::expect_assign(key_value_callback_t callback) {
	return _expect_type<ast::AssignStatement>([callback](ast::AssignStatement const* assign_node) -> bool {
		std::string_view left;
		bool ret = expect_identifier(assign_variable_callback(left))(assign_node->left());
		if (ret) {
			ret &= callback(left, assign_node->right());
			if (!ret) {
				Logger::error("Callback failed for assign node with key: ", left);
			}
		} else {
			Logger::error("Callback key failed for assign node with key: ", left);
		}
		return ret;
	});
}

node_callback_t NodeTools::expect_list_and_length(length_callback_t length_callback, node_callback_t callback) {
	return _abstract_statement_node_callback([length_callback, callback](_NodeStatementRange list) -> bool {
		bool ret = true;
		auto dist = ranges::distance(list);
		size_t size = length_callback(dist);

		if (size > dist) {
			Logger::error("Trying to read more values than the list contains: ", size, " > ", dist);
			size = dist;
			ret = false;
		}
		for (auto [index, sub_node] : list | ranges::views::enumerate) {
			if (index >= size) {
				break;
			}
			if (auto const* value = dryad::node_try_cast<ast::ValueStatement>(sub_node)) {
				ret &= callback(value->value());
				continue;
			}
			ret &= callback(sub_node);
		}
		return ret;
	});
}

node_callback_t NodeTools::expect_list_of_length(size_t length, node_callback_t callback) {
	return [length, callback](ast::NodeCPtr node) -> bool {
		bool ret = true;
		ret &= expect_list_and_length(
			[length, &ret](size_t size) -> size_t {
				if (size != length) {
					Logger::error("List length ", size, " does not match expected length ", length);
					ret = false;
					if (length < size) {
						return length;
					}
				}
				return size;
			},
			callback
		)(node);
		return ret;
	};
}

node_callback_t NodeTools::expect_list(node_callback_t callback) {
	return expect_list_and_length(default_length_callback, callback);
}

node_callback_t NodeTools::expect_length(callback_t<size_t> callback) {
	return [callback](ast::NodeCPtr node) -> bool {
		bool ret = true;
		ret &= expect_list_and_length(
			[callback, &ret](size_t size) -> size_t {
				ret &= callback(size);
				return 0;
			},
			success_callback
		)(node);
		return ret;
	};
}

template<typename Key>
static node_callback_t _expect_key(Key key, NodeCallback auto callback, bool* key_found, bool allow_duplicates) {
	if constexpr (std::same_as<Key, ovdl::symbol<char>>) {
		if (!key) {
			if (key_found != nullptr) {
				*key_found = false;
			}
			return [](ast::NodeCPtr) -> bool {
				Logger::error("Failed to find expected interned key.");
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

	return _abstract_statement_node_callback([key, callback, key_found, allow_duplicates](_NodeStatementRange list) -> bool {
		bool ret = true;
		size_t keys_found = 0;
		for (auto sub_node : list) {
			auto const* assign_node = dryad::node_try_cast<ast::AssignStatement>(sub_node);
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
				Logger::error("Failed to find expected key: \"", key_str, "\"");
			}
			ret = false;
		} else {
			if (key_found != nullptr) {
				*key_found = true;
			}
			if (!allow_duplicates && keys_found > 1) {
				Logger::error("Found ", keys_found, " instances of key: \"", key_str, "\" (expected 1)");
				ret = false;
			}
		}
		return ret;
	});
}

node_callback_t NodeTools::expect_key(std::string_view key, node_callback_t callback, bool* key_found, bool allow_duplicates) {
	return _expect_key(key, callback, key_found, allow_duplicates);
}

node_callback_t
NodeTools::expect_key(ovdl::symbol<char> key, node_callback_t callback, bool* key_found, bool allow_duplicates) {
	return _expect_key(key, callback, key_found, allow_duplicates);
}

node_callback_t NodeTools::expect_dictionary_and_length(length_callback_t length_callback, key_value_callback_t callback) {
	return expect_list_and_length(length_callback, expect_assign(callback));
}

node_callback_t NodeTools::expect_dictionary(key_value_callback_t callback) {
	return expect_dictionary_and_length(default_length_callback, callback);
}

node_callback_t NodeTools::name_list_callback(callback_t<name_list_t&&> callback) {
	return [callback](ast::NodeCPtr node) -> bool {
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
		std::for_each(name_list.begin() + 1, name_list.end(), [&stream](std::string const& name) -> void {
			stream << ", " << name;
		});
	}
	return stream << ']';
}

callback_t<std::string_view> NodeTools::assign_variable_callback_string(std::string& var) {
	return assign_variable_callback_cast<std::string_view>(var);
}
