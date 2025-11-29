#include "openvic-simulation/dataloader/NodeTools.hpp"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <string_view>
#include <system_error>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>
#include <dryad/node_map.hpp>
#include <dryad/tree.hpp>

#include <range/v3/iterator/operations.hpp>
#include <range/v3/numeric/iota.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/core/string/CharConv.hpp"

#include "../types/Colour.hpp" // IWYU pragma: keep
#include "Helper.hpp" // IWYU pragma: keep
#include "types/Numeric.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::ast;
using namespace std::string_view_literals;

namespace snitch {
	template<typename T>
	inline static bool append(snitch::small_string_span ss, ovdl::symbol<T> const& s) {
		return append(ss, s.view());
	}
}

struct Ast : ovdl::SymbolIntern {
	dryad::tree<OpenVic::ast::FileTree> tree;
	dryad::node_map<const Node, ovdl::NodeLocation> map;
	symbol_interner_type symbol_interner;

	template<typename T, typename... Args>
	T* create(Args&&... args) {
		auto node = tree.template create<T>(DRYAD_FWD(args)...);
		return node;
	}

	template<typename T, typename... Args>
	T* create_with_intern(std::string_view view, Args&&... args) {
		auto intern = symbol_interner.intern(view.data(), view.size());
		auto node = tree.template create<T>(intern, DRYAD_FWD(args)...);
		return node;
	}

	template<typename T, typename... Args>
	T* create_with_loc(ovdl::NodeLocation loc, Args&&... args) {
		auto node = tree.template create<T>(DRYAD_FWD(args)...);
		set_location(node, loc);
		return node;
	}

	template<typename T, typename... Args>
	T* create_with_loc_and_intern(ovdl::NodeLocation loc, std::string_view view, Args&&... args) {
		auto intern = symbol_interner.intern(view.data(), view.size());
		auto node = tree.template create<T>(intern, DRYAD_FWD(args)...);
		set_location(node, loc);
		return node;
	}

	void set_location(const Node* n, ovdl::NodeLocation loc) {
		map.insert(n, loc);
	}

	ovdl::NodeLocation location_of(const Node* n) const {
		auto result = map.lookup(n);
		if (result == nullptr) {
			return {};
		}
		return *result;
	}


	template<typename T = IdentifierValue>
	inline auto create_value_statement_factory() {
		return [this](std::string_view sv) {
			return create<ValueStatement>(create_with_intern<T>(sv));
		};
	}

	template<typename T = IdentifierValue>
	inline auto create_value_statement_factory(std::string_view sv) {
		return [this, sv]() {
			return create_value_statement_factory<T>()(sv);
		};
	}

	template<typename LeftT, typename RightT>
	inline auto create_assign_statement_factory(std::string_view lhs, std::string_view rhs) {
		return [this, lhs, rhs]() {
			return create<AssignStatement>(create_with_intern<LeftT>(lhs), create_with_intern<RightT>(rhs));
		};
	}

	template<typename RightT = IdentifierValue>
	inline auto create_assign_statement_factory(std::string_view lhs, std::string_view rhs) {
		return create_assign_statement_factory<IdentifierValue, RightT>(lhs, rhs);
	}

	template<typename RightT = IdentifierValue>
	inline auto create_assign_statement_rhs_factory(std::string_view rhs) {
		return [this, rhs](std::string_view lhs) {
			return create_assign_statement_factory<IdentifierValue, RightT>(lhs, rhs)();
		};
	}

	inline ListValue* create_value_list_fill(size_t count, auto&& factory) {
		StatementList slist;
		for (size_t index = 0; index < count; index++) {
			slist.push_back(factory());
		}
		return create<ListValue>(slist);
	}

	inline ListValue* create_value_list(auto&&... factories) {
		StatementList slist;
		(slist.push_back(factories()), ...);
		return create<ListValue>(slist);
	}

	inline ListValue* create_assign_list(std::span<const std::string_view> span, auto&& factory) {
		StatementList slist;
		for (std::string_view const& sv : span) {
			if constexpr (requires { factory(sv); }) {
				slist.push_back(factory(sv));
			} else {
				slist.push_back(factory());
			}
		}
		return create<ListValue>(slist);
	}

	template<typename T = IdentifierValue>
	inline ListValue* create_assign_list(std::span<const std::string_view> span_lhs, std::span<std::string_view> span_rhs) {
		StatementList slist;
		for (auto [index, lhs] : span_lhs | ranges::views::enumerate) {
			index %= span_rhs.size();
			slist.push_back(create_assign_statement_rhs_factory<T>(span_rhs[index])(lhs));
		}
		return create<ListValue>(slist);
	}

	inline ListValue* create_assign_list(size_t count, auto&& factory) {
		static constexpr size_t bits_per_digit = 4;
		static constexpr size_t array_length = sizeof(size_t) * CHAR_BIT / bits_per_digit * 4;
		thread_local std::array<char, array_length> hex_array {};

		StatementList slist;
		for (size_t index : ranges::views::iota(static_cast<size_t>(0), count)) {
			std::to_chars_result result = std::to_chars(hex_array.data(), hex_array.data() + hex_array.size(), index);
			CHECK_IF(result.ec == std::errc {});
			else {
				continue;
			}
			slist.push_back(factory(std::string_view { hex_array.data(), result.ptr }));
		}
		return create<ListValue>(slist);
	}

	template<typename T = IdentifierValue>
	inline ListValue* create_assign_list(size_t count, std::span<std::string_view> span_rhs) {
		static constexpr size_t bits_per_digit = 4;
		static constexpr size_t array_length = sizeof(size_t) * CHAR_BIT / bits_per_digit * 4;
		thread_local std::array<char, array_length> hex_array {};

		StatementList slist;
		for (auto [index, lhs] : ranges::views::iota(static_cast<size_t>(0), count) | ranges::views::enumerate) {
			index %= span_rhs.size();
			std::to_chars_result result = std::to_chars(hex_array.data(), hex_array.data() + hex_array.size(), lhs);
			CHECK_IF(result.ec == std::errc {});
			else {
				continue;
			}
			slist.push_back(create_assign_statement_rhs_factory<T>(span_rhs[static_cast<size_t>(index)])(lhs));
		}
		return create<ListValue>(slist);
	}
};

TEST_CASE("NodeTools AST get_type_name", "[NodeTools][NodeTools-ast-get_type_name]") {
	CONSTEXPR_CHECK( //
		ast::get_type_name(ovdl::v2script::ast::NodeKind::FileTree) == "ovdl::v2script::ast::FileTree"sv
	);
	CONSTEXPR_CHECK(
		ast::get_type_name(ovdl::v2script::ast::NodeKind::IdentifierValue) == "ovdl::v2script::ast::IdentifierValue"sv
	);
	CONSTEXPR_CHECK( //
		ast::get_type_name(ovdl::v2script::ast::NodeKind::StringValue) == "ovdl::v2script::ast::StringValue"sv
	);
	CONSTEXPR_CHECK( //
		ast::get_type_name(ovdl::v2script::ast::NodeKind::ListValue) == "ovdl::v2script::ast::ListValue"sv
	);
	CONSTEXPR_CHECK( //
		ast::get_type_name(ovdl::v2script::ast::NodeKind::NullValue) == "ovdl::v2script::ast::NullValue"sv
	);
	CONSTEXPR_CHECK(
		ast::get_type_name(ovdl::v2script::ast::NodeKind::EventStatement) == "ovdl::v2script::ast::EventStatement"sv
	);
	CONSTEXPR_CHECK(
		ast::get_type_name(ovdl::v2script::ast::NodeKind::AssignStatement) == "ovdl::v2script::ast::AssignStatement"sv
	);
	CONSTEXPR_CHECK(
		ast::get_type_name(ovdl::v2script::ast::NodeKind::ValueStatement) == "ovdl::v2script::ast::ValueStatement"sv
	);
}

TEST_CASE("NodeTools Default Callback functions", "[NodeTools][NodeTools-default-callback-functions]") {
	dryad::tree<Node> tree;
	Node* null_value = tree.create<NullValue>();

	CHECK(NodeTools::success_callback(null_value));
	CHECK(NodeTools::key_value_success_callback(""sv, null_value));
	CHECK_FALSE(NodeTools::key_value_invalid_callback(""sv, null_value));
	CONSTEXPR_CHECK(NodeTools::default_length_callback(0) == 0);
	CONSTEXPR_CHECK(NodeTools::default_length_callback(5) == 5);
	CONSTEXPR_CHECK(
		NodeTools::default_length_callback(std::numeric_limits<std::size_t>::max()) == std::numeric_limits<std::size_t>::max()
	);
}

TEST_CASE("NodeTools expect string functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-string-functions]") {
	Ast ast;
	auto* id = ast.create_with_intern<IdentifierValue>("expect"sv);
	auto* str = ast.create_with_intern<StringValue>("expect"sv);

	NodeTools::Callback<std::string_view> auto string_view_callback = [](std::string_view sv) -> bool {
		CHECK_RETURN_BOOL(sv == "expect"sv);
	};

	CHECK(NodeTools::expect_identifier(string_view_callback)(id));
	CHECK_FALSE(NodeTools::expect_string(string_view_callback)(id));

	CHECK_FALSE(NodeTools::expect_identifier(string_view_callback)(str));
	CHECK(NodeTools::expect_string(string_view_callback)(str));

	CHECK(NodeTools::expect_identifier_or_string(string_view_callback)(str));
	CHECK(NodeTools::expect_identifier_or_string(string_view_callback)(id));
}

TEST_CASE("NodeTools expect bool functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-bool-functions]") {
	Ast ast;

	auto* yes_id = ast.create_with_intern<IdentifierValue>("yes"sv);
	auto* yes_str = ast.create_with_intern<StringValue>("yes"sv);
	auto* no_id = ast.create_with_intern<IdentifierValue>("no"sv);
	auto* no_str = ast.create_with_intern<StringValue>("no"sv);

	auto* _2_id = ast.create_with_intern<IdentifierValue>("2"sv);
	auto* _2_str = ast.create_with_intern<StringValue>("2"sv);
	auto* _1_id = ast.create_with_intern<IdentifierValue>("1"sv);
	auto* _1_str = ast.create_with_intern<StringValue>("1"sv);
	auto* _0_id = ast.create_with_intern<IdentifierValue>("0"sv);
	auto* _0_str = ast.create_with_intern<StringValue>("0"sv);

	NodeTools::Callback<bool> auto bool_callback = [](bool val) -> bool {
		return val;
	};

	CHECK(NodeTools::expect_bool(bool_callback)(yes_id));
	CHECK_FALSE(NodeTools::expect_bool(bool_callback)(yes_str));
	CHECK_FALSE(NodeTools::expect_bool(bool_callback)(no_id));
	CHECK_FALSE(NodeTools::expect_bool(bool_callback)(no_str));

	CHECK_FALSE(NodeTools::expect_bool(bool_callback)(_2_id));
	CHECK_FALSE(NodeTools::expect_bool(bool_callback)(_2_str));
	CHECK_FALSE(NodeTools::expect_bool(bool_callback)(_1_id));
	CHECK_FALSE(NodeTools::expect_bool(bool_callback)(_1_str));
	CHECK_FALSE(NodeTools::expect_bool(bool_callback)(_0_id));
	CHECK_FALSE(NodeTools::expect_bool(bool_callback)(_0_str));

	CHECK_FALSE(NodeTools::expect_int_bool(bool_callback)(yes_id));
	CHECK_FALSE(NodeTools::expect_int_bool(bool_callback)(yes_str));
	CHECK_FALSE(NodeTools::expect_int_bool(bool_callback)(no_id));
	CHECK_FALSE(NodeTools::expect_int_bool(bool_callback)(no_str));

	CHECK(NodeTools::expect_int_bool(bool_callback)(_2_id));
	CHECK_FALSE(NodeTools::expect_int_bool(bool_callback)(_2_str));
	CHECK(NodeTools::expect_int_bool(bool_callback)(_1_id));
	CHECK_FALSE(NodeTools::expect_int_bool(bool_callback)(_1_str));
	CHECK_FALSE(NodeTools::expect_int_bool(bool_callback)(_0_id));
	CHECK_FALSE(NodeTools::expect_int_bool(bool_callback)(_0_str));
}

TEST_CASE("NodeTools expect integer functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-integer-functions]") {
	Ast ast;

	auto* _2_id = ast.create_with_intern<IdentifierValue>("2"sv);
	auto* _2_str = ast.create_with_intern<StringValue>("2"sv);
	auto* _1_id = ast.create_with_intern<IdentifierValue>("1"sv);
	auto* _1_str = ast.create_with_intern<StringValue>("1"sv);
	auto* _0_id = ast.create_with_intern<IdentifierValue>("0"sv);
	auto* _0_str = ast.create_with_intern<StringValue>("0"sv);

	auto* _2002_id = ast.create_with_intern<IdentifierValue>("2002"sv);
	auto* _2002_str = ast.create_with_intern<StringValue>("2002"sv);
	auto* _1001_id = ast.create_with_intern<IdentifierValue>("1001"sv);
	auto* _1001_str = ast.create_with_intern<StringValue>("1001"sv);

	auto* _max_long_id = ast.create_with_intern<IdentifierValue>("9223372036854775807"sv);
	auto* _max_long_str = ast.create_with_intern<StringValue>("9223372036854775807"sv);
	auto* _max_ulong_id = ast.create_with_intern<IdentifierValue>("18446744073709551615"sv);
	auto* _max_ulong_str = ast.create_with_intern<StringValue>("18446744073709551615"sv);
	auto* _min_long_id = ast.create_with_intern<IdentifierValue>("-9223372036854775808"sv);
	auto* _min_long_str = ast.create_with_intern<StringValue>("-9223372036854775808"sv);

	auto callback = [](FlatValue const* ptr, auto val) -> bool {
		std::string_view sv = ptr->value().view();

		decltype(val) check;
		std::from_chars_result result = std::from_chars(sv.data(), sv.data() + sv.size(), check);
		CHECK_IF(result.ec == std::errc {});
		else {
			return false;
		}

		CHECK_RETURN_BOOL(check == val);
	};

	CHECK(NodeTools::expect_int64(std::bind_front(callback, _2_id))(_2_id));
	CHECK_FALSE(NodeTools::expect_int64(std::bind_front(callback, _2_str))(_2_str));
	CHECK(NodeTools::expect_int64(std::bind_front(callback, _1_id))(_1_id));
	CHECK_FALSE(NodeTools::expect_int64(std::bind_front(callback, _1_str))(_1_str));
	CHECK(NodeTools::expect_int64(std::bind_front(callback, _0_id))(_0_id));
	CHECK_FALSE(NodeTools::expect_int64(std::bind_front(callback, _0_str))(_0_str));
	CHECK(NodeTools::expect_int64(std::bind_front(callback, _2002_id))(_2002_id));
	CHECK_FALSE(NodeTools::expect_int64(std::bind_front(callback, _2002_str))(_2002_str));
	CHECK(NodeTools::expect_int64(std::bind_front(callback, _1001_id))(_1001_id));
	CHECK_FALSE(NodeTools::expect_int64(std::bind_front(callback, _1001_str))(_1001_str));

	CHECK(NodeTools::expect_int64(std::bind_front(callback, _max_long_id))(_max_long_id));
	CHECK_FALSE(NodeTools::expect_int64(std::bind_front(callback, _max_long_str))(_max_long_str));
	CHECK_FALSE(NodeTools::expect_int64(std::bind_front(callback, _max_ulong_id))(_max_ulong_id));
	CHECK_FALSE(NodeTools::expect_int64(std::bind_front(callback, _max_ulong_str))(_max_ulong_str));
	CHECK(NodeTools::expect_int64(std::bind_front(callback, _min_long_id))(_min_long_id));
	CHECK_FALSE(NodeTools::expect_int64(std::bind_front(callback, _min_long_str))(_min_long_str));

	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _2_id))(_2_id));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(callback, _2_str))(_2_str));
	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _1_id))(_1_id));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(callback, _1_str))(_1_str));
	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _0_id))(_0_id));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(callback, _0_str))(_0_str));
	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _2002_id))(_2002_id));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(callback, _2002_str))(_2002_str));
	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _1001_id))(_1001_id));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(callback, _1001_str))(_1001_str));

	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _max_long_id))(_max_long_id));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(callback, _max_long_str))(_max_long_str));
	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _max_ulong_id))(_max_ulong_id));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(callback, _max_ulong_str))(_max_ulong_str));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(callback, _min_long_id))(_min_long_id));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(callback, _min_long_str))(_min_long_str));

	auto base_callback = [](FlatValue const* ptr, int base, auto val) -> bool {
		std::string_view sv = ptr->value().view();

		decltype(val) check;
		std::from_chars_result result = string_to_uint64(sv, check, base);
		CHECK_IF(result.ec == std::errc {});
		else {
			return false;
		}

		CHECK_RETURN_BOOL(check == val);
	};

	auto* _binary_id = ast.create_with_intern<IdentifierValue>("0110"sv);
	auto* _binary_str = ast.create_with_intern<StringValue>("0110"sv);

	CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _binary_id, 2), 2)(_binary_id));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(base_callback, _binary_str, 2), 2)(_binary_str));

	auto* _octal_id = ast.create_with_intern<IdentifierValue>("2674"sv);
	auto* _octal_str = ast.create_with_intern<StringValue>("2674"sv);

	CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _octal_id, 8), 8)(_octal_id));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(base_callback, _octal_str, 8), 8)(_octal_str));

	SECTION("Detect Octal Base") {
		auto* _detect_octal_id = ast.create_with_intern<IdentifierValue>("02674"sv);
		auto* _detect_octal_str = ast.create_with_intern<StringValue>("02674"sv);

		CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _detect_octal_id, 0), 0)(_detect_octal_id));
		CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(base_callback, _detect_octal_str, 0), 0)(_detect_octal_str));

		CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _detect_octal_id, 8), 0)(_detect_octal_id));
		CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(base_callback, _detect_octal_str, 8), 0)(_detect_octal_str));
	}

	auto* _hex_id = ast.create_with_intern<IdentifierValue>("0xff2e5"sv);
	auto* _hex_str = ast.create_with_intern<StringValue>("0xff2e5"sv);

	CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _hex_id, 16), 16)(_hex_id));
	CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(base_callback, _hex_str, 16), 16)(_hex_str));

	SECTION("Detect Hexadecimal Base") {
		CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _hex_id, 0), 0)(_hex_id));
		CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(base_callback, _hex_str, 0), 0)(_hex_str));

		CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _hex_id, 16), 0)(_hex_id));
		CHECK_FALSE(NodeTools::expect_uint64(std::bind_front(base_callback, _hex_str, 16), 0)(_hex_str));
	}
}

TEST_CASE(
	"NodeTools expect fixed_point_t functions",
	"[NodeTools][NodeTools-expect-functions][NodeTools-expect-fixed_point_t-functions]"
) {
	Ast ast;

	auto* _0_01_id = ast.create_with_intern<IdentifierValue>("0.01"sv);
	auto* _0_01_str = ast.create_with_intern<StringValue>("0.01"sv);
	auto* _2_21_id = ast.create_with_intern<IdentifierValue>("2.21"sv);
	auto* _2_21_str = ast.create_with_intern<StringValue>("2.21"sv);
	auto* _303_id = ast.create_with_intern<IdentifierValue>("303"sv);
	auto* _303_str = ast.create_with_intern<StringValue>("303"sv);

	auto callback = [](FlatValue const* ptr, fixed_point_t val) -> bool {
		std::string_view sv = ptr->value().view();

		fixed_point_t check = 0;
		std::from_chars_result result = check.from_chars(sv.data(), sv.data() + sv.size());
		CHECK_IF(result.ec == std::errc {});
		else {
			return false;
		}

		CHECK_RETURN_BOOL(check == val);
	};

	CHECK(NodeTools::expect_fixed_point(std::bind_front(callback, _0_01_id))(_0_01_id));
	CHECK_FALSE(NodeTools::expect_fixed_point(std::bind_front(callback, _0_01_str))(_0_01_str));
	CHECK(NodeTools::expect_fixed_point(std::bind_front(callback, _2_21_id))(_2_21_id));
	CHECK_FALSE(NodeTools::expect_fixed_point(std::bind_front(callback, _2_21_str))(_2_21_str));
	CHECK(NodeTools::expect_fixed_point(std::bind_front(callback, _303_id))(_303_id));
	CHECK_FALSE(NodeTools::expect_fixed_point(std::bind_front(callback, _303_str))(_303_str));

	CHECK_FALSE(NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _0_01_id)))(_0_01_id));
	CHECK(NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _0_01_str)))(_0_01_str));
	CHECK_FALSE(NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _2_21_id)))(_2_21_id));
	CHECK(NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _2_21_str)))(_2_21_str));
	CHECK_FALSE(NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _303_id)))(_303_id));
	CHECK(NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _303_str)))(_303_str));
}

TEST_CASE("NodeTools expect colour functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-colour-functions]") {
	Ast ast;

	auto _0 = ast.create_value_statement_factory("0"sv);
	auto _0_5 = ast.create_value_statement_factory("0.5"sv);
	auto _1 = ast.create_value_statement_factory("1"sv);
	auto _127 = ast.create_value_statement_factory("127"sv);
	auto _255 = ast.create_value_statement_factory("255"sv);

	auto* _0_list = ast.create_value_list_fill(3, _0);
	auto* _0_5_list = ast.create_value_list_fill(3, _0_5);
	auto* _1_list = ast.create_value_list_fill(3, _1);
	auto* _127_list = ast.create_value_list_fill(3, _127);
	auto* _255_list = ast.create_value_list_fill(3, _255);

	auto* _mix_list = ast.create_value_list(_0, _0_5, _255);

	auto callback = [](ListValue const* ptr, colour_t val) -> bool {
		auto list = ptr->statements();
		CHECK_IF(ranges::distance(list) == 3);
		else {
			return false;
		}

		colour_t check = colour_t::null();
		for (auto [index, sub_node] : list | ranges::views::enumerate) {
			if (auto const* value = dryad::node_try_cast<ValueStatement>(sub_node)) {
				if (auto const* id = dryad::node_try_cast<IdentifierValue>(value->value())) {
					std::string_view sv = id->value().view();
					fixed_point_t fp = 0;
					std::from_chars_result result = fp.from_chars(sv.data(), sv.data() + sv.size());
					CHECK_OR_CONTINUE(result.ec == std::errc {});
					if (fp <= 1) {
						fp *= 255;
					}
					check[index] = fp.truncate<colour_t::value_type>();
				}
			}
		}

		CHECK_RETURN_BOOL(check == val);
	};

	CHECK(NodeTools::expect_colour(std::bind_front(callback, _0_list))(_0_list));
	CHECK(NodeTools::expect_colour(std::bind_front(callback, _0_5_list))(_0_5_list));
	CHECK(NodeTools::expect_colour(std::bind_front(callback, _1_list))(_1_list));
	CHECK(NodeTools::expect_colour(std::bind_front(callback, _127_list))(_127_list));
	CHECK(NodeTools::expect_colour(std::bind_front(callback, _255_list))(_255_list));
	CHECK(NodeTools::expect_colour(std::bind_front(callback, _mix_list))(_mix_list));
}

TEST_CASE(
	"NodeTools expect colour hex functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-colour-hex-functions]"
) {
	Ast ast;

	auto* orange_id = ast.create_with_intern<IdentifierValue>("0xFFBB55"sv);
	auto* orange_str = ast.create_with_intern<StringValue>("0xFFBB55"sv);

	auto callback = [](FlatValue const* ptr, colour_argb_t val) -> bool {
		std::string_view sv = ptr->value().view();

		colour_argb_t check;
		std::from_chars_result result = check.from_chars_argb(sv.data(), sv.data() + sv.size());
		CHECK_IF(result.ec == std::errc {});
		else {
			return false;
		}

		CHECK_RETURN_BOOL(check == val);
	};

	CHECK(NodeTools::expect_colour_hex(std::bind_front(callback, orange_id))(orange_id));
	CHECK_FALSE(NodeTools::expect_colour_hex(std::bind_front(callback, orange_str))(orange_str));
}

TEST_CASE(
	"NodeTools expect text format functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-text-format-functions]"
) {
	Ast ast;

	auto* left_id = ast.create_with_intern<IdentifierValue>("left"sv);
	auto* left_str = ast.create_with_intern<StringValue>("left"sv);
	auto* right_id = ast.create_with_intern<IdentifierValue>("right"sv);
	auto* right_str = ast.create_with_intern<StringValue>("right"sv);
	auto* centre_id = ast.create_with_intern<IdentifierValue>("centre"sv);
	auto* centre_str = ast.create_with_intern<StringValue>("centre"sv);
	auto* center_id = ast.create_with_intern<IdentifierValue>("center"sv);
	auto* center_str = ast.create_with_intern<StringValue>("center"sv);
	auto* justified_id = ast.create_with_intern<IdentifierValue>("justified"sv);
	auto* justified_str = ast.create_with_intern<StringValue>("justified"sv);

	auto callback = [](FlatValue const* ptr, text_format_t val) -> bool {
		using enum text_format_t;
		static const string_map_t<text_format_t> format_map = //
			{ //
			  { "left", left }, //
			  { "right", right },
			  { "centre", centre },
			  { "center", centre },
			  { "justified", justified }
			};

		std::string_view sv = ptr->value().view();
		text_format_t check = format_map.find(sv).value();

		CHECK_RETURN_BOOL(check == val);
	};

	CHECK(NodeTools::expect_text_format(std::bind_front(callback, left_id))(left_id));
	CHECK_FALSE(NodeTools::expect_text_format(std::bind_front(callback, left_str))(left_str));
	CHECK(NodeTools::expect_text_format(std::bind_front(callback, right_id))(right_id));
	CHECK_FALSE(NodeTools::expect_text_format(std::bind_front(callback, right_str))(right_str));
	CHECK(NodeTools::expect_text_format(std::bind_front(callback, centre_id))(centre_id));
	CHECK_FALSE(NodeTools::expect_text_format(std::bind_front(callback, centre_str))(centre_str));
	CHECK(NodeTools::expect_text_format(std::bind_front(callback, center_id))(center_id));
	CHECK_FALSE(NodeTools::expect_text_format(std::bind_front(callback, center_str))(center_str));
	CHECK(NodeTools::expect_text_format(std::bind_front(callback, justified_id))(justified_id));
	CHECK_FALSE(NodeTools::expect_text_format(std::bind_front(callback, justified_str))(justified_str));
}

TEST_CASE("NodeTools expect Date functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-date-functions]") {
	Ast ast;

	auto* date_id = ast.create_with_intern<IdentifierValue>("1.1.1"sv);
	auto* date_str = ast.create_with_intern<StringValue>("1.1.1"sv);

	static auto sv_callback = [](std::string_view sv, Date val) -> bool {
		Date::from_chars_result result = {};
		Date check = Date::from_string(sv, &result);
		CHECK_IF(result.ec == std::errc {});
		else {
			return false;
		}

		CHECK_RETURN_BOOL(check == val);
	};

	auto callback = [](FlatValue const* ptr, Date val) -> bool {
		std::string_view sv = ptr->value().view();
		return sv_callback(sv, val);
	};

	CHECK(NodeTools::expect_date(std::bind_front(callback, date_id))(date_id));
	CHECK_FALSE(NodeTools::expect_date(std::bind_front(callback, date_str))(date_str));
	CHECK_FALSE(NodeTools::expect_date_string(std::bind_front(callback, date_id))(date_id));
	CHECK(NodeTools::expect_date_string(std::bind_front(callback, date_str))(date_str));
	CHECK(NodeTools::expect_date_identifier_or_string(std::bind_front(callback, date_id))(date_id));
	CHECK(NodeTools::expect_date_identifier_or_string(std::bind_front(callback, date_str))(date_str));

	CHECK(NodeTools::expect_date_str(std::bind_front(callback, date_id))(date_id->value().view()));
	CHECK(NodeTools::expect_date_str(std::bind_front(callback, date_str))(date_str->value().view()));
}

TEST_CASE(
	"NodeTools expect Timespan functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-timespan-functions]"
) {
	Ast ast;

	auto* _5_id = ast.create_with_intern<IdentifierValue>("5"sv);
	auto* _5_str = ast.create_with_intern<StringValue>("5"sv);

	enum class TimespanType : uint8_t { Day, Month, Year };
	auto callback = [](FlatValue const* ptr, TimespanType type, Timespan val) -> bool {
		std::string_view sv = ptr->value().view();

		int64_t check_int;
		std::from_chars_result result = string_to_int64(sv, check_int);
		CHECK_IF(result.ec == std::errc {});
		else {
			return false;
		}

		Timespan check = [&] {
			switch (type) {
			case TimespanType::Day:	  return Timespan::from_days(check_int);
			case TimespanType::Month: return Timespan::from_months(check_int);
			case TimespanType::Year:  return Timespan::from_years(check_int);
			}
			return Timespan {};
		}();

		CHECK_RETURN_BOOL(check == val);
	};

	CHECK(NodeTools::expect_years(std::bind_front(callback, _5_id, TimespanType::Year))(_5_id));
	CHECK_FALSE(NodeTools::expect_years(std::bind_front(callback, _5_str, TimespanType::Year))(_5_str));
	CHECK(NodeTools::expect_months(std::bind_front(callback, _5_id, TimespanType::Month))(_5_id));
	CHECK_FALSE(NodeTools::expect_months(std::bind_front(callback, _5_str, TimespanType::Month))(_5_str));
	CHECK(NodeTools::expect_days(std::bind_front(callback, _5_id, TimespanType::Day))(_5_id));
	CHECK_FALSE(NodeTools::expect_days(std::bind_front(callback, _5_str, TimespanType::Day))(_5_str));
}

TEST_CASE("NodeTools expect vector functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-vector-functions]") {
	Ast ast;

	auto _0_id_assign = ast.create_assign_statement_rhs_factory("0"sv);
	auto _0_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("0"sv);
	auto _0_5_id_assign = ast.create_assign_statement_rhs_factory("0.5"sv);
	auto _0_5_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("0.5"sv);
	auto _1_id_assign = ast.create_assign_statement_rhs_factory("1"sv);
	auto _1_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("1"sv);
	auto _127_id_assign = ast.create_assign_statement_rhs_factory("127"sv);
	auto _127_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("127"sv);
	auto _255_id_assign = ast.create_assign_statement_rhs_factory("255"sv);
	auto _255_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("255"sv);

	static auto callback = [](ListValue const* ptr, auto val) -> bool {
		auto list = ptr->statements();
		CHECK_IF(ranges::distance(list) == val.size());
		else {
			return false;
		}

		bool callback_return = true;
		for (auto [index, statement] : list | ranges::views::enumerate) {
			std::string_view sv;

			if (auto* assign = dryad::node_try_cast<AssignStatement>(statement)) {
				auto* lhs = dryad::node_cast<FlatValue>(assign->left());
				auto* rhs = dryad::node_cast<FlatValue>(assign->right());
				sv = rhs->value().view();

				CAPTURE(index);
				CAPTURE(sv);

				switch (index) {
				case 0: CHECK(lhs->value().view() == "x"sv); break;
				case 1: CHECK(lhs->value().view() == "y"sv); break;
				case 2: CHECK(lhs->value().view() == "z"sv); break;
				case 3: CHECK(lhs->value().view() == "w"sv); break;
				}
			} else {
				auto* value = dryad::node_cast<ValueStatement>(statement);
				auto* flat_value = dryad::node_cast<FlatValue>(value->value());
				sv = flat_value->value().view();
			}

			typename decltype(val)::type check;
			std::from_chars_result result = [&] {
				if constexpr (std::same_as<decltype(check), fixed_point_t>) {
					return check.from_chars(sv.data(), sv.data() + sv.size());
				} else {
					return from_chars(sv.data(), sv.data() + sv.size(), check);
				}
			}();
			CHECK_OR_CONTINUE(result.ec == std::errc {});
			CHECK_OR_CONTINUE(check == val[index]);
		}

		return callback_return;
	};

	static const std::array xy = std::array { "x"sv, "y"sv };
	ListValue* _0_id_2_list = ast.create_assign_list(xy, _0_id_assign);
	ListValue* _0_str_2_list = ast.create_assign_list(xy, _0_str_assign);
	ListValue* _0_5_id_2_list = ast.create_assign_list(xy, _0_5_id_assign);
	ListValue* _0_5_str_2_list = ast.create_assign_list(xy, _0_5_str_assign);
	ListValue* _1_id_2_list = ast.create_assign_list(xy, _1_id_assign);
	ListValue* _1_str_2_list = ast.create_assign_list(xy, _1_str_assign);
	ListValue* _127_id_2_list = ast.create_assign_list(xy, _127_id_assign);
	ListValue* _127_str_2_list = ast.create_assign_list(xy, _127_str_assign);
	ListValue* _255_id_2_list = ast.create_assign_list(xy, _255_id_assign);
	ListValue* _255_str_2_list = ast.create_assign_list(xy, _255_str_assign);

	CHECK(NodeTools::expect_ivec2(std::bind_front(callback, _0_id_2_list))(_0_id_2_list));
	CHECK(NodeTools::expect_ivec2(std::bind_front(callback, _0_5_id_2_list))(_0_5_id_2_list));
	CHECK(NodeTools::expect_ivec2(std::bind_front(callback, _1_id_2_list))(_1_id_2_list));
	CHECK(NodeTools::expect_ivec2(std::bind_front(callback, _127_id_2_list))(_127_id_2_list));
	CHECK(NodeTools::expect_ivec2(std::bind_front(callback, _255_id_2_list))(_255_id_2_list));

	CHECK_FALSE(NodeTools::expect_ivec2(std::bind_front(callback, _0_str_2_list))(_0_str_2_list));
	CHECK_FALSE(NodeTools::expect_ivec2(std::bind_front(callback, _0_5_str_2_list))(_0_5_str_2_list));
	CHECK_FALSE(NodeTools::expect_ivec2(std::bind_front(callback, _1_str_2_list))(_1_str_2_list));
	CHECK_FALSE(NodeTools::expect_ivec2(std::bind_front(callback, _127_str_2_list))(_127_str_2_list));
	CHECK_FALSE(NodeTools::expect_ivec2(std::bind_front(callback, _255_str_2_list))(_255_str_2_list));

	CHECK(NodeTools::expect_fvec2(std::bind_front(callback, _0_id_2_list))(_0_id_2_list));
	CHECK(NodeTools::expect_fvec2(std::bind_front(callback, _0_5_id_2_list))(_0_5_id_2_list));
	CHECK(NodeTools::expect_fvec2(std::bind_front(callback, _1_id_2_list))(_1_id_2_list));
	CHECK(NodeTools::expect_fvec2(std::bind_front(callback, _127_id_2_list))(_127_id_2_list));
	CHECK(NodeTools::expect_fvec2(std::bind_front(callback, _255_id_2_list))(_255_id_2_list));

	CHECK_FALSE(NodeTools::expect_fvec2(std::bind_front(callback, _0_str_2_list))(_0_str_2_list));
	CHECK_FALSE(NodeTools::expect_fvec2(std::bind_front(callback, _0_5_str_2_list))(_0_5_str_2_list));
	CHECK_FALSE(NodeTools::expect_fvec2(std::bind_front(callback, _1_str_2_list))(_1_str_2_list));
	CHECK_FALSE(NodeTools::expect_fvec2(std::bind_front(callback, _127_str_2_list))(_127_str_2_list));
	CHECK_FALSE(NodeTools::expect_fvec2(std::bind_front(callback, _255_str_2_list))(_255_str_2_list));

	auto _0_id = ast.create_value_statement_factory("0"sv);
	auto _0_str = ast.create_value_statement_factory<StringValue>("0"sv);
	auto _0_5_id = ast.create_value_statement_factory("0.5"sv);
	auto _0_5_str = ast.create_value_statement_factory<StringValue>("0.5"sv);
	auto _1_id = ast.create_value_statement_factory("1"sv);
	auto _1_str = ast.create_value_statement_factory<StringValue>("1"sv);
	auto _127_id = ast.create_value_statement_factory("127"sv);
	auto _127_str = ast.create_value_statement_factory<StringValue>("127"sv);
	auto _255_id = ast.create_value_statement_factory("255"sv);
	auto _255_str = ast.create_value_statement_factory<StringValue>("255"sv);

	ListValue* _0_id_3_list = ast.create_value_list_fill(3, _0_id);
	ListValue* _0_str_3_list = ast.create_value_list_fill(3, _0_str);
	ListValue* _0_5_id_3_list = ast.create_value_list_fill(3, _0_5_id);
	ListValue* _0_5_str_3_list = ast.create_value_list_fill(3, _0_5_str);
	ListValue* _1_id_3_list = ast.create_value_list_fill(3, _1_id);
	ListValue* _1_str_3_list = ast.create_value_list_fill(3, _1_str);
	ListValue* _127_id_3_list = ast.create_value_list_fill(3, _127_id);
	ListValue* _127_str_3_list = ast.create_value_list_fill(3, _127_str);
	ListValue* _255_id_3_list = ast.create_value_list_fill(3, _255_id);
	ListValue* _255_str_3_list = ast.create_value_list_fill(3, _255_str);

	CHECK(NodeTools::expect_fvec3(std::bind_front(callback, _0_id_3_list))(_0_id_3_list));
	CHECK(NodeTools::expect_fvec3(std::bind_front(callback, _0_5_id_3_list))(_0_5_id_3_list));
	CHECK(NodeTools::expect_fvec3(std::bind_front(callback, _1_id_3_list))(_1_id_3_list));
	CHECK(NodeTools::expect_fvec3(std::bind_front(callback, _127_id_3_list))(_127_id_3_list));
	CHECK(NodeTools::expect_fvec3(std::bind_front(callback, _255_id_3_list))(_255_id_3_list));

	CHECK_FALSE(NodeTools::expect_fvec3(std::bind_front(callback, _0_str_3_list))(_0_str_3_list));
	CHECK_FALSE(NodeTools::expect_fvec3(std::bind_front(callback, _0_5_str_3_list))(_0_5_str_3_list));
	CHECK_FALSE(NodeTools::expect_fvec3(std::bind_front(callback, _1_str_3_list))(_1_str_3_list));
	CHECK_FALSE(NodeTools::expect_fvec3(std::bind_front(callback, _127_str_3_list))(_127_str_3_list));
	CHECK_FALSE(NodeTools::expect_fvec3(std::bind_front(callback, _255_str_3_list))(_255_str_3_list));

	ListValue* _0_id_4_list = ast.create_value_list_fill(4, _0_id);
	ListValue* _0_str_4_list = ast.create_value_list_fill(4, _0_str);
	ListValue* _0_5_id_4_list = ast.create_value_list_fill(4, _0_5_id);
	ListValue* _0_5_str_4_list = ast.create_value_list_fill(4, _0_5_str);
	ListValue* _1_id_4_list = ast.create_value_list_fill(4, _1_id);
	ListValue* _1_str_4_list = ast.create_value_list_fill(4, _1_str);
	ListValue* _127_id_4_list = ast.create_value_list_fill(4, _127_id);
	ListValue* _127_str_4_list = ast.create_value_list_fill(4, _127_str);
	ListValue* _255_id_4_list = ast.create_value_list_fill(4, _255_id);
	ListValue* _255_str_4_list = ast.create_value_list_fill(4, _255_str);

	CHECK(NodeTools::expect_fvec4(std::bind_front(callback, _0_id_4_list))(_0_id_4_list));
	CHECK(NodeTools::expect_fvec4(std::bind_front(callback, _0_5_id_4_list))(_0_5_id_4_list));
	CHECK(NodeTools::expect_fvec4(std::bind_front(callback, _1_id_4_list))(_1_id_4_list));
	CHECK(NodeTools::expect_fvec4(std::bind_front(callback, _127_id_4_list))(_127_id_4_list));
	CHECK(NodeTools::expect_fvec4(std::bind_front(callback, _255_id_4_list))(_255_id_4_list));

	CHECK_FALSE(NodeTools::expect_fvec4(std::bind_front(callback, _0_str_4_list))(_0_str_4_list));
	CHECK_FALSE(NodeTools::expect_fvec4(std::bind_front(callback, _0_5_str_4_list))(_0_5_str_4_list));
	CHECK_FALSE(NodeTools::expect_fvec4(std::bind_front(callback, _1_str_4_list))(_1_str_4_list));
	CHECK_FALSE(NodeTools::expect_fvec4(std::bind_front(callback, _127_str_4_list))(_127_str_4_list));
	CHECK_FALSE(NodeTools::expect_fvec4(std::bind_front(callback, _255_str_4_list))(_255_str_4_list));
}

TEST_CASE("NodeTools expect assign functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-assign-functions]") {
	Ast ast;

	auto* id = ast.create_assign_statement_rhs_factory("right"sv)("left"sv);
	auto* str = ast.create_assign_statement_rhs_factory<StringValue>("right"sv)("left"sv);

	auto callback = [](AssignStatement const* ptr, std::string_view lhs, NodeCPtr rhs) -> bool {
		auto* ptr_lhs = dryad::node_cast<FlatValue>(ptr->left());
		auto* ptr_rhs = dryad::node_cast<FlatValue>(ptr->right());

		auto* value = dryad::node_cast<FlatValue>(rhs);

		CHECK_IF(ptr_lhs->value().view() == lhs) {
			CHECK_IF(ptr_rhs->value().view() == value->value().view()) {
				return true;
			}
		}
		return false;
	};

	CHECK(NodeTools::expect_assign(std::bind_front(callback, id))(id));
	CHECK(NodeTools::expect_assign(std::bind_front(callback, str))(str));
}

TEST_CASE("NodeTools expect list functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-list-functions]") {
	static constexpr size_t bits_per_digit = 4;
	static constexpr size_t array_length = sizeof(size_t) * CHAR_BIT / bits_per_digit * 4;
	static std::array<char, array_length> hex_array {};

	Ast ast;

	auto id_value = ast.create_value_statement_factory();
	auto str_value = ast.create_value_statement_factory<StringValue>();

	auto* _0_id_list = ast.create_assign_list(0, id_value);
	auto* _0_str_list = ast.create_assign_list(0, str_value);
	auto* _3_id_list = ast.create_assign_list(3, id_value);
	auto* _3_str_list = ast.create_assign_list(3, str_value);
	auto* _4_id_list = ast.create_assign_list(4, id_value);
	auto* _4_str_list = ast.create_assign_list(4, str_value);
	auto* _10_id_list = ast.create_assign_list(10, id_value);
	auto* _10_str_list = ast.create_assign_list(10, str_value);
	auto* _24_id_list = ast.create_assign_list(24, id_value);
	auto* _24_str_list = ast.create_assign_list(24, str_value);

	static auto length_callback = [](size_t size, size_t check) {
		CHECK_IF(size == check) {
			return true;
		}
		return false;
	};

	static auto callback = [](ListValue const* ptr, size_t count, NodeCPtr val) -> bool {
		auto list = ptr->statements();
		CHECK_IF(ranges::distance(list) == count);
		else {
			return false;
		}

		for (auto [index, statement] : list | ranges::views::enumerate) {
			auto* check_value = dryad::node_cast<ValueStatement>(statement);

			auto* flat_value = dryad::node_cast<FlatValue>(val);

			auto* check_flat_value = dryad::node_cast<FlatValue>(check_value->value());
			std::string_view check = check_flat_value->value().view();

			CAPTURE(index);
			CAPTURE(check);

			if (check_flat_value != flat_value) {
				CHECK(count > index);
				continue;
			}

			std::string_view value = flat_value->value().view();

			CAPTURE(value);

			CHECK_RETURN_BOOL(check == value);
		}

		CHECK_RETURN_BOOL(count == 0);
	};

	// clang-format off
	CHECK(
		NodeTools::expect_list_and_length(NodeTools::default_length_callback, std::bind_front(callback, _0_id_list, 0))(_0_id_list)
	);
	CHECK(
		NodeTools::expect_list_and_length(NodeTools::default_length_callback, std::bind_front(callback, _0_str_list, 0))(_0_str_list)
	);
	CHECK(
		NodeTools::expect_list_and_length(NodeTools::default_length_callback, std::bind_front(callback, _3_id_list, 3))(_3_id_list)
	);
	CHECK(
		NodeTools::expect_list_and_length(NodeTools::default_length_callback, std::bind_front(callback, _3_str_list, 3))(_3_str_list)
	);
	CHECK(
		NodeTools::expect_list_and_length(NodeTools::default_length_callback, std::bind_front(callback, _4_id_list, 4))(_4_id_list)
	);
	CHECK(
		NodeTools::expect_list_and_length(NodeTools::default_length_callback, std::bind_front(callback, _4_str_list, 4))(_4_str_list)
	);
	CHECK(
		NodeTools::expect_list_and_length(NodeTools::default_length_callback, std::bind_front(callback, _10_id_list, 10))(_10_id_list)
	);
	CHECK(
		NodeTools::expect_list_and_length(NodeTools::default_length_callback, std::bind_front(callback, _10_str_list, 10))(_10_str_list)
	);
	CHECK(
		NodeTools::expect_list_and_length(NodeTools::default_length_callback, std::bind_front(callback, _24_id_list, 24))(_24_id_list)
	);
	CHECK(
		NodeTools::expect_list_and_length(NodeTools::default_length_callback, std::bind_front(callback, _24_str_list, 24))(_24_str_list)
	);
	// clang-format on

	CHECK(NodeTools::expect_list_and_length([](size_t) -> size_t { return 1;}, std::bind_front(callback, _24_str_list, 24))(_24_str_list));
	CHECK(NodeTools::expect_list_and_length([](size_t) -> size_t { return 24;}, std::bind_front(callback, _24_str_list, 24))(_24_str_list));
	CHECK_FALSE(NodeTools::expect_list_and_length([](size_t) -> size_t { return 25;}, std::bind_front(callback, _24_str_list, 24))(_24_str_list));

	CHECK(NodeTools::expect_list_of_length(0, std::bind_front(callback, _0_id_list, 0))(_0_id_list));
	CHECK(NodeTools::expect_list_of_length(0, std::bind_front(callback, _0_str_list, 0))(_0_str_list));
	CHECK(NodeTools::expect_list_of_length(3, std::bind_front(callback, _3_id_list, 3))(_3_id_list));
	CHECK(NodeTools::expect_list_of_length(3, std::bind_front(callback, _3_str_list, 3))(_3_str_list));
	CHECK(NodeTools::expect_list_of_length(4, std::bind_front(callback, _4_id_list, 4))(_4_id_list));
	CHECK(NodeTools::expect_list_of_length(4, std::bind_front(callback, _4_str_list, 4))(_4_str_list));
	CHECK(NodeTools::expect_list_of_length(10, std::bind_front(callback, _10_id_list, 10))(_10_id_list));
	CHECK(NodeTools::expect_list_of_length(10, std::bind_front(callback, _10_str_list, 10))(_10_str_list));
	CHECK(NodeTools::expect_list_of_length(24, std::bind_front(callback, _24_id_list, 24))(_24_id_list));
	CHECK(NodeTools::expect_list_of_length(24, std::bind_front(callback, _24_str_list, 24))(_24_str_list));

	CHECK_FALSE(NodeTools::expect_list_of_length(25, std::bind_front(callback, _24_id_list, 24))(_24_id_list));
	CHECK_FALSE(NodeTools::expect_list_of_length(25, std::bind_front(callback, _24_str_list, 24))(_24_str_list));

	CHECK_FALSE(NodeTools::expect_list_of_length(23, std::bind_front(callback, _24_id_list, 24))(_24_id_list));
	CHECK_FALSE(NodeTools::expect_list_of_length(23, std::bind_front(callback, _24_str_list, 24))(_24_str_list));

	CHECK(NodeTools::expect_list(std::bind_front(callback, _0_id_list, 0))(_0_id_list));
	CHECK(NodeTools::expect_list(std::bind_front(callback, _0_str_list, 0))(_0_str_list));
	CHECK(NodeTools::expect_list(std::bind_front(callback, _3_id_list, 3))(_3_id_list));
	CHECK(NodeTools::expect_list(std::bind_front(callback, _3_str_list, 3))(_3_str_list));
	CHECK(NodeTools::expect_list(std::bind_front(callback, _4_id_list, 4))(_4_id_list));
	CHECK(NodeTools::expect_list(std::bind_front(callback, _4_str_list, 4))(_4_str_list));
	CHECK(NodeTools::expect_list(std::bind_front(callback, _10_id_list, 10))(_10_id_list));
	CHECK(NodeTools::expect_list(std::bind_front(callback, _10_str_list, 10))(_10_str_list));
	CHECK(NodeTools::expect_list(std::bind_front(callback, _24_id_list, 24))(_24_id_list));
	CHECK(NodeTools::expect_list(std::bind_front(callback, _24_str_list, 24))(_24_str_list));
}

TEST_CASE("NodeTools expect length functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-length-functions]") {
	Ast ast;

	auto id_value = ast.create_value_statement_factory();
	auto str_value = ast.create_value_statement_factory<StringValue>();

	auto* _0_id_list = ast.create_assign_list(0, id_value);
	auto* _0_str_list = ast.create_assign_list(0, str_value);
	auto* _3_id_list = ast.create_assign_list(3, id_value);
	auto* _3_str_list = ast.create_assign_list(3, str_value);
	auto* _4_id_list = ast.create_assign_list(4, id_value);
	auto* _4_str_list = ast.create_assign_list(4, str_value);
	auto* _10_id_list = ast.create_assign_list(10, id_value);
	auto* _10_str_list = ast.create_assign_list(10, str_value);
	auto* _24_id_list = ast.create_assign_list(24, id_value);
	auto* _24_str_list = ast.create_assign_list(24, str_value);

	static auto callback = [](ListValue const* ptr, size_t count, size_t val) -> bool {
		auto list = ptr->statements();
		CHECK_IF(ranges::distance(list) == count);
		else {
			return false;
		}

		CHECK_RETURN_BOOL(count == val);
	};

	CHECK(NodeTools::expect_length(std::bind_front(callback, _0_id_list, 0))(_0_id_list));
	CHECK(NodeTools::expect_length(std::bind_front(callback, _0_str_list, 0))(_0_str_list));
	CHECK(NodeTools::expect_length(std::bind_front(callback, _3_id_list, 3))(_3_id_list));
	CHECK(NodeTools::expect_length(std::bind_front(callback, _3_str_list, 3))(_3_str_list));
	CHECK(NodeTools::expect_length(std::bind_front(callback, _4_id_list, 4))(_4_id_list));
	CHECK(NodeTools::expect_length(std::bind_front(callback, _4_str_list, 4))(_4_str_list));
	CHECK(NodeTools::expect_length(std::bind_front(callback, _10_id_list, 10))(_10_id_list));
	CHECK(NodeTools::expect_length(std::bind_front(callback, _10_str_list, 10))(_10_str_list));
	CHECK(NodeTools::expect_length(std::bind_front(callback, _24_id_list, 24))(_24_id_list));
	CHECK(NodeTools::expect_length(std::bind_front(callback, _24_str_list, 24))(_24_str_list));

	static auto callback_false = [](ListValue const* ptr, size_t count, size_t val) -> bool {
		auto list = ptr->statements();
		CHECK_FALSE_IF(ranges::distance(list) == count);
		else {
			return false;
		}

		CHECK_FALSE_RETURN_BOOL(count == val);
	};

	CHECK_FALSE(NodeTools::expect_length(std::bind_front(callback_false, _0_id_list, 1))(_0_id_list));
	CHECK_FALSE(NodeTools::expect_length(std::bind_front(callback_false, _0_str_list, 1))(_0_str_list));
	CHECK_FALSE(NodeTools::expect_length(std::bind_front(callback_false, _3_id_list, 4))(_3_id_list));
	CHECK_FALSE(NodeTools::expect_length(std::bind_front(callback_false, _3_str_list, 4))(_3_str_list));
	CHECK_FALSE(NodeTools::expect_length(std::bind_front(callback_false, _4_id_list, 3))(_4_id_list));
	CHECK_FALSE(NodeTools::expect_length(std::bind_front(callback_false, _4_str_list, 3))(_4_str_list));
	CHECK_FALSE(NodeTools::expect_length(std::bind_front(callback_false, _10_id_list, 20))(_10_id_list));
	CHECK_FALSE(NodeTools::expect_length(std::bind_front(callback_false, _10_str_list, 20))(_10_str_list));
	CHECK_FALSE(NodeTools::expect_length(std::bind_front(callback_false, _24_id_list, 0))(_24_id_list));
	CHECK_FALSE(NodeTools::expect_length(std::bind_front(callback_false, _24_str_list, 0))(_24_str_list));
}

TEST_CASE("NodeTools expect key functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-key-functions]") {
	static constexpr size_t bits_per_digit = 4;
	static constexpr size_t array_length = sizeof(size_t) * CHAR_BIT / bits_per_digit * 4;
	thread_local std::array<char, array_length> hex_array {};

	Ast ast;

	auto _0_id_assign = ast.create_assign_statement_rhs_factory("0"sv);
	auto _0_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("0"sv);
	auto _0_5_id_assign = ast.create_assign_statement_rhs_factory("0.5"sv);
	auto _0_5_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("0.5"sv);
	auto _1_id_assign = ast.create_assign_statement_rhs_factory("1"sv);
	auto _1_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("1"sv);
	auto _127_id_assign = ast.create_assign_statement_rhs_factory("127"sv);
	auto _127_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("127"sv);
	auto _255_id_assign = ast.create_assign_statement_rhs_factory("255"sv);
	auto _255_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("255"sv);

	static auto callback = [](ListValue const* ptr, NodeCPtr val) -> bool {
		auto* flat_val = dryad::node_cast<FlatValue>(val);
		std::string_view val_sv = flat_val->value().view();
		CAPTURE(val_sv);

		auto list = ptr->statements();

		bool callback_return = false;
		for (auto [index, statement] : list | ranges::views::enumerate) {
			auto* assign = dryad::node_cast<AssignStatement>(statement);
			auto* lhs = dryad::node_cast<FlatValue>(assign->left());
			auto* rhs = dryad::node_cast<FlatValue>(assign->right());

			if (flat_val != rhs) {
				continue;
			}

			std::string_view lhs_sv = lhs->value().view();
			std::string_view rhs_sv = rhs->value().view();

			CAPTURE(index);
			CAPTURE(lhs_sv);
			CAPTURE(rhs_sv);

			size_t check;
			std::from_chars_result result = std::from_chars(lhs_sv.data(), lhs_sv.data() + lhs_sv.size(), check);

			CHECK_OR_CONTINUE(result.ec == std::errc {});
			CHECK_OR_CONTINUE(check == index);
			CHECK_OR_CONTINUE(rhs->value() == flat_val->value());
			CHECK_RETURN_BOOL(val_sv == rhs_sv);
		}

		CHECK_RETURN_BOOL(callback_return);
	};

	ListValue* _0_id_list = ast.create_assign_list(8, _0_id_assign);
	ListValue* _0_str_list = ast.create_assign_list(8, _0_str_assign);
	ListValue* _0_5_id_list = ast.create_assign_list(7, _0_5_id_assign);
	ListValue* _0_5_str_list = ast.create_assign_list(7, _0_5_str_assign);
	ListValue* _1_id_list = ast.create_assign_list(6, _1_id_assign);
	ListValue* _1_str_list = ast.create_assign_list(6, _1_str_assign);
	ListValue* _127_id_list = ast.create_assign_list(5, _127_id_assign);
	ListValue* _127_str_list = ast.create_assign_list(5, _127_str_assign);
	ListValue* _255_id_list = ast.create_assign_list(4, _255_id_assign);
	ListValue* _255_str_list = ast.create_assign_list(4, _255_str_assign);

	ovdl::symbol _0 = ast.symbol_interner.find_intern("0");
	ovdl::symbol _1 = ast.symbol_interner.find_intern("1");
	ovdl::symbol _2 = ast.symbol_interner.find_intern("2");
	ovdl::symbol _3 = ast.symbol_interner.find_intern("3");
	ovdl::symbol _4 = ast.symbol_interner.find_intern("4");
	ovdl::symbol _5 = ast.symbol_interner.find_intern("5");
	ovdl::symbol _6 = ast.symbol_interner.find_intern("6");
	ovdl::symbol _7 = ast.symbol_interner.find_intern("7");

	CHECK(NodeTools::expect_key(_7, std::bind_front(callback, _0_id_list))(_0_id_list));
	CHECK(NodeTools::expect_key(_7, std::bind_front(callback, _0_str_list))(_0_str_list));
	CHECK(NodeTools::expect_key(_6, std::bind_front(callback, _0_5_id_list))(_0_5_id_list));
	CHECK(NodeTools::expect_key(_6, std::bind_front(callback, _0_5_str_list))(_0_5_str_list));
	CHECK(NodeTools::expect_key(_5, std::bind_front(callback, _1_id_list))(_1_id_list));
	CHECK(NodeTools::expect_key(_5, std::bind_front(callback, _1_str_list))(_1_str_list));
	CHECK(NodeTools::expect_key(_4, std::bind_front(callback, _127_id_list))(_127_id_list));
	CHECK(NodeTools::expect_key(_4, std::bind_front(callback, _127_str_list))(_127_str_list));
	CHECK(NodeTools::expect_key(_3, std::bind_front(callback, _255_id_list))(_255_id_list));
	CHECK(NodeTools::expect_key(_3, std::bind_front(callback, _255_str_list))(_255_str_list));
	CHECK(NodeTools::expect_key(_2, std::bind_front(callback, _255_id_list))(_255_id_list));
	CHECK(NodeTools::expect_key(_2, std::bind_front(callback, _255_str_list))(_255_str_list));
	CHECK(NodeTools::expect_key(_1, std::bind_front(callback, _255_id_list))(_255_id_list));
	CHECK(NodeTools::expect_key(_1, std::bind_front(callback, _255_str_list))(_255_str_list));
	CHECK(NodeTools::expect_key(_0, std::bind_front(callback, _255_id_list))(_255_id_list));
	CHECK(NodeTools::expect_key(_0, std::bind_front(callback, _255_str_list))(_255_str_list));

	CHECK(NodeTools::expect_key(_7.view(), std::bind_front(callback, _0_id_list))(_0_id_list));
	CHECK(NodeTools::expect_key(_7.view(), std::bind_front(callback, _0_str_list))(_0_str_list));
	CHECK(NodeTools::expect_key(_6.view(), std::bind_front(callback, _0_5_id_list))(_0_5_id_list));
	CHECK(NodeTools::expect_key(_6.view(), std::bind_front(callback, _0_5_str_list))(_0_5_str_list));
	CHECK(NodeTools::expect_key(_5.view(), std::bind_front(callback, _1_id_list))(_1_id_list));
	CHECK(NodeTools::expect_key(_5.view(), std::bind_front(callback, _1_str_list))(_1_str_list));
	CHECK(NodeTools::expect_key(_4.view(), std::bind_front(callback, _127_id_list))(_127_id_list));
	CHECK(NodeTools::expect_key(_4.view(), std::bind_front(callback, _127_str_list))(_127_str_list));
	CHECK(NodeTools::expect_key(_3.view(), std::bind_front(callback, _255_id_list))(_255_id_list));
	CHECK(NodeTools::expect_key(_3.view(), std::bind_front(callback, _255_str_list))(_255_str_list));
	CHECK(NodeTools::expect_key(_2.view(), std::bind_front(callback, _255_id_list))(_255_id_list));
	CHECK(NodeTools::expect_key(_2.view(), std::bind_front(callback, _255_str_list))(_255_str_list));
	CHECK(NodeTools::expect_key(_1.view(), std::bind_front(callback, _255_id_list))(_255_id_list));
	CHECK(NodeTools::expect_key(_1.view(), std::bind_front(callback, _255_str_list))(_255_str_list));
	CHECK(NodeTools::expect_key(_0.view(), std::bind_front(callback, _255_id_list))(_255_id_list));
	CHECK(NodeTools::expect_key(_0.view(), std::bind_front(callback, _255_str_list))(_255_str_list));

	CHECK(NodeTools::expect_key(_7, std::bind_front(callback, _0_id_list), nullptr, true)(_0_id_list));
	CHECK(NodeTools::expect_key(_7, std::bind_front(callback, _0_str_list), nullptr, true)(_0_str_list));
	CHECK(NodeTools::expect_key(_6, std::bind_front(callback, _0_5_id_list), nullptr, true)(_0_5_id_list));
	CHECK(NodeTools::expect_key(_6, std::bind_front(callback, _0_5_str_list), nullptr, true)(_0_5_str_list));
	CHECK(NodeTools::expect_key(_5, std::bind_front(callback, _1_id_list), nullptr, true)(_1_id_list));
	CHECK(NodeTools::expect_key(_5, std::bind_front(callback, _1_str_list), nullptr, true)(_1_str_list));
	CHECK(NodeTools::expect_key(_4, std::bind_front(callback, _127_id_list), nullptr, true)(_127_id_list));
	CHECK(NodeTools::expect_key(_4, std::bind_front(callback, _127_str_list), nullptr, true)(_127_str_list));
	CHECK(NodeTools::expect_key(_3, std::bind_front(callback, _255_id_list), nullptr, true)(_255_id_list));
	CHECK(NodeTools::expect_key(_3, std::bind_front(callback, _255_str_list), nullptr, true)(_255_str_list));
	CHECK(NodeTools::expect_key(_2, std::bind_front(callback, _255_id_list), nullptr, true)(_255_id_list));
	CHECK(NodeTools::expect_key(_2, std::bind_front(callback, _255_str_list), nullptr, true)(_255_str_list));
	CHECK(NodeTools::expect_key(_1, std::bind_front(callback, _255_id_list), nullptr, true)(_255_id_list));
	CHECK(NodeTools::expect_key(_1, std::bind_front(callback, _255_str_list), nullptr, true)(_255_str_list));
	CHECK(NodeTools::expect_key(_0, std::bind_front(callback, _255_id_list), nullptr, true)(_255_id_list));
	CHECK(NodeTools::expect_key(_0, std::bind_front(callback, _255_str_list), nullptr, true)(_255_str_list));

	CHECK(NodeTools::expect_key(_7.view(), std::bind_front(callback, _0_id_list), nullptr, true)(_0_id_list));
	CHECK(NodeTools::expect_key(_7.view(), std::bind_front(callback, _0_str_list), nullptr, true)(_0_str_list));
	CHECK(NodeTools::expect_key(_6.view(), std::bind_front(callback, _0_5_id_list), nullptr, true)(_0_5_id_list));
	CHECK(NodeTools::expect_key(_6.view(), std::bind_front(callback, _0_5_str_list), nullptr, true)(_0_5_str_list));
	CHECK(NodeTools::expect_key(_5.view(), std::bind_front(callback, _1_id_list), nullptr, true)(_1_id_list));
	CHECK(NodeTools::expect_key(_5.view(), std::bind_front(callback, _1_str_list), nullptr, true)(_1_str_list));
	CHECK(NodeTools::expect_key(_4.view(), std::bind_front(callback, _127_id_list), nullptr, true)(_127_id_list));
	CHECK(NodeTools::expect_key(_4.view(), std::bind_front(callback, _127_str_list), nullptr, true)(_127_str_list));
	CHECK(NodeTools::expect_key(_3.view(), std::bind_front(callback, _255_id_list), nullptr, true)(_255_id_list));
	CHECK(NodeTools::expect_key(_3.view(), std::bind_front(callback, _255_str_list), nullptr, true)(_255_str_list));
	CHECK(NodeTools::expect_key(_2.view(), std::bind_front(callback, _255_id_list), nullptr, true)(_255_id_list));
	CHECK(NodeTools::expect_key(_2.view(), std::bind_front(callback, _255_str_list), nullptr, true)(_255_str_list));
	CHECK(NodeTools::expect_key(_1.view(), std::bind_front(callback, _255_id_list), nullptr, true)(_255_id_list));
	CHECK(NodeTools::expect_key(_1.view(), std::bind_front(callback, _255_str_list), nullptr, true)(_255_str_list));
	CHECK(NodeTools::expect_key(_0.view(), std::bind_front(callback, _255_id_list), nullptr, true)(_255_id_list));
	CHECK(NodeTools::expect_key(_0.view(), std::bind_front(callback, _255_str_list), nullptr, true)(_255_str_list));

	static auto callback_dup = [](ListValue const* ptr, NodeCPtr val) -> bool {
		auto* flat_val = dryad::node_cast<FlatValue>(val);
		std::string_view val_sv = flat_val->value().view();
		CAPTURE(val_sv);

		auto list = ptr->statements();

		static auto transformer = [](ranges::common_pair<size_t, Statement const*> pair) -> decltype(pair) {
			pair.first /= 2;
			return pair;
		};
		for (auto [index, statement] : list | ranges::views::enumerate | ranges::views::transform(transformer)) {
			auto* assign = dryad::node_cast<AssignStatement>(statement);
			auto* lhs = dryad::node_cast<FlatValue>(assign->left());
			auto* rhs = dryad::node_cast<FlatValue>(assign->right());

			if (flat_val != rhs) {
				continue;
			}

			std::string_view rhs_sv = rhs->value().view();
			std::string_view lhs_sv = lhs->value().view();

			CAPTURE(index);
			CAPTURE(rhs_sv);
			CAPTURE(lhs_sv);

			size_t check;
			std::from_chars_result result = std::from_chars(lhs_sv.data(), lhs_sv.data() + lhs_sv.size(), check);

			CHECK_OR_CONTINUE(result.ec == std::errc {});
			CHECK_OR_CONTINUE(check == index);
			CHECK(rhs->value() == flat_val->value());
			CHECK_RETURN_BOOL(val_sv == rhs_sv);
		}

		CHECK_RETURN_BOOL(false);
	};

	auto build_list_dup = [&ast](size_t count, auto&& factory) -> ListValue* {
		StatementList slist;
		for (size_t index : ranges::views::iota(static_cast<size_t>(0), count)) {
			std::to_chars_result result = std::to_chars(hex_array.data(), hex_array.data() + hex_array.size(), index);
			CHECK_OR_CONTINUE(result.ec == std::errc {});
			slist.push_back(factory(std::string_view { hex_array.data(), result.ptr }));
			slist.push_back(factory(std::string_view { hex_array.data(), result.ptr }));
		}
		return ast.create<ListValue>(slist);
	};

	_0_id_list = build_list_dup(12, _0_id_assign);
	_0_str_list = build_list_dup(12, _0_str_assign);
	_0_5_id_list = build_list_dup(11, _0_5_id_assign);
	_0_5_str_list = build_list_dup(11, _0_5_str_assign);
	_1_id_list = build_list_dup(10, _1_id_assign);
	_1_str_list = build_list_dup(10, _1_str_assign);
	_127_id_list = build_list_dup(9, _127_id_assign);
	_127_str_list = build_list_dup(9, _127_str_assign);
	_255_id_list = build_list_dup(8, _255_id_assign);
	_255_str_list = build_list_dup(8, _255_str_assign);

	ovdl::symbol _8 = ast.symbol_interner.find_intern("8");
	ovdl::symbol _9 = ast.symbol_interner.find_intern("9");
	ovdl::symbol _10 = ast.symbol_interner.find_intern("10");
	ovdl::symbol _11 = ast.symbol_interner.find_intern("11");

	CHECK_FALSE(NodeTools::expect_key(_11, std::bind_front(callback_dup, _0_id_list))(_0_id_list));
	CHECK_FALSE(NodeTools::expect_key(_11, std::bind_front(callback_dup, _0_str_list))(_0_str_list));
	CHECK_FALSE(NodeTools::expect_key(_10, std::bind_front(callback_dup, _0_5_id_list))(_0_5_id_list));
	CHECK_FALSE(NodeTools::expect_key(_10, std::bind_front(callback_dup, _0_5_str_list))(_0_5_str_list));
	CHECK_FALSE(NodeTools::expect_key(_9, std::bind_front(callback_dup, _1_id_list))(_1_id_list));
	CHECK_FALSE(NodeTools::expect_key(_9, std::bind_front(callback_dup, _1_str_list))(_1_str_list));
	CHECK_FALSE(NodeTools::expect_key(_8, std::bind_front(callback_dup, _127_id_list))(_127_id_list));
	CHECK_FALSE(NodeTools::expect_key(_8, std::bind_front(callback_dup, _127_str_list))(_127_str_list));
	CHECK_FALSE(NodeTools::expect_key(_7, std::bind_front(callback_dup, _255_id_list))(_255_id_list));
	CHECK_FALSE(NodeTools::expect_key(_7, std::bind_front(callback_dup, _255_str_list))(_255_str_list));

	CHECK_FALSE(NodeTools::expect_key(_11.view(), std::bind_front(callback_dup, _0_id_list))(_0_id_list));
	CHECK_FALSE(NodeTools::expect_key(_11.view(), std::bind_front(callback_dup, _0_str_list))(_0_str_list));
	CHECK_FALSE(NodeTools::expect_key(_10.view(), std::bind_front(callback_dup, _0_5_id_list))(_0_5_id_list));
	CHECK_FALSE(NodeTools::expect_key(_10.view(), std::bind_front(callback_dup, _0_5_str_list))(_0_5_str_list));
	CHECK_FALSE(NodeTools::expect_key(_9.view(), std::bind_front(callback_dup, _1_id_list))(_1_id_list));
	CHECK_FALSE(NodeTools::expect_key(_9.view(), std::bind_front(callback_dup, _1_str_list))(_1_str_list));
	CHECK_FALSE(NodeTools::expect_key(_8.view(), std::bind_front(callback_dup, _127_id_list))(_127_id_list));
	CHECK_FALSE(NodeTools::expect_key(_8.view(), std::bind_front(callback_dup, _127_str_list))(_127_str_list));
	CHECK_FALSE(NodeTools::expect_key(_7.view(), std::bind_front(callback_dup, _255_id_list))(_255_id_list));
	CHECK_FALSE(NodeTools::expect_key(_7.view(), std::bind_front(callback_dup, _255_str_list))(_255_str_list));

	CHECK(NodeTools::expect_key(_11, std::bind_front(callback_dup, _0_id_list), nullptr, true)(_0_id_list));
	CHECK(NodeTools::expect_key(_11, std::bind_front(callback_dup, _0_str_list), nullptr, true)(_0_str_list));
	CHECK(NodeTools::expect_key(_10, std::bind_front(callback_dup, _0_5_id_list), nullptr, true)(_0_5_id_list));
	CHECK(NodeTools::expect_key(_10, std::bind_front(callback_dup, _0_5_str_list), nullptr, true)(_0_5_str_list));
	CHECK(NodeTools::expect_key(_9, std::bind_front(callback_dup, _1_id_list), nullptr, true)(_1_id_list));
	CHECK(NodeTools::expect_key(_9, std::bind_front(callback_dup, _1_str_list), nullptr, true)(_1_str_list));
	CHECK(NodeTools::expect_key(_8, std::bind_front(callback_dup, _127_id_list), nullptr, true)(_127_id_list));
	CHECK(NodeTools::expect_key(_8, std::bind_front(callback_dup, _127_str_list), nullptr, true)(_127_str_list));
	CHECK(NodeTools::expect_key(_7, std::bind_front(callback_dup, _255_id_list), nullptr, true)(_255_id_list));
	CHECK(NodeTools::expect_key(_7, std::bind_front(callback_dup, _255_str_list), nullptr, true)(_255_str_list));

	CHECK(NodeTools::expect_key(_11.view(), std::bind_front(callback_dup, _0_id_list), nullptr, true)(_0_id_list));
	CHECK(NodeTools::expect_key(_11.view(), std::bind_front(callback_dup, _0_str_list), nullptr, true)(_0_str_list));
	CHECK(NodeTools::expect_key(_10.view(), std::bind_front(callback_dup, _0_5_id_list), nullptr, true)(_0_5_id_list));
	CHECK(NodeTools::expect_key(_10.view(), std::bind_front(callback_dup, _0_5_str_list), nullptr, true)(_0_5_str_list));
	CHECK(NodeTools::expect_key(_9.view(), std::bind_front(callback_dup, _1_id_list), nullptr, true)(_1_id_list));
	CHECK(NodeTools::expect_key(_9.view(), std::bind_front(callback_dup, _1_str_list), nullptr, true)(_1_str_list));
	CHECK(NodeTools::expect_key(_8.view(), std::bind_front(callback_dup, _127_id_list), nullptr, true)(_127_id_list));
	CHECK(NodeTools::expect_key(_8.view(), std::bind_front(callback_dup, _127_str_list), nullptr, true)(_127_str_list));
	CHECK(NodeTools::expect_key(_7.view(), std::bind_front(callback_dup, _255_id_list), nullptr, true)(_255_id_list));
	CHECK(NodeTools::expect_key(_7.view(), std::bind_front(callback_dup, _255_str_list), nullptr, true)(_255_str_list));
}

TEST_CASE(
	"NodeTools expect dictionary functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-dictionary-functions]"
) {
	Ast ast;

	auto _0_id_assign = ast.create_assign_statement_rhs_factory("0"sv);
	auto _0_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("0"sv);
	auto _0_5_id_assign = ast.create_assign_statement_rhs_factory("0.5"sv);
	auto _0_5_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("0.5"sv);
	auto _1_id_assign = ast.create_assign_statement_rhs_factory("1"sv);
	auto _1_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("1"sv);
	auto _127_id_assign = ast.create_assign_statement_rhs_factory("127"sv);
	auto _127_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("127"sv);
	auto _255_id_assign = ast.create_assign_statement_rhs_factory("255"sv);
	auto _255_str_assign = ast.create_assign_statement_rhs_factory<StringValue>("255"sv);

	static auto callback = [](ListValue const* ptr, std::string_view key, NodeCPtr val) -> bool {
		CAPTURE(key);
		auto* flat_val = dryad::node_cast<FlatValue>(val);
		std::string_view val_sv = flat_val->value().view();
		CAPTURE(val_sv);

		auto list = ptr->statements();

		for (auto [index, statement] : list | ranges::views::enumerate) {
			auto* assign = dryad::node_cast<AssignStatement>(statement);
			auto* lhs = dryad::node_cast<FlatValue>(assign->left());
			auto* rhs = dryad::node_cast<FlatValue>(assign->right());

			if (flat_val != rhs) {
				continue;
			}

			std::string_view lhs_sv = lhs->value().view();
			std::string_view rhs_sv = rhs->value().view();

			CAPTURE(index);
			CAPTURE(lhs_sv);
			CAPTURE(rhs_sv);

			size_t check;
			std::from_chars_result result = std::from_chars(lhs_sv.data(), lhs_sv.data() + lhs_sv.size(), check);

			CHECK_OR_CONTINUE(result.ec == std::errc {});
			CHECK_OR_CONTINUE(check == index);
			CHECK_OR_CONTINUE(lhs_sv == key);
			CHECK_OR_CONTINUE(rhs->value() == flat_val->value());
			CHECK_RETURN_BOOL(val_sv == rhs_sv);
		}

		CHECK_RETURN_BOOL(false);
	};

	ListValue* _0_id_list = ast.create_assign_list(8, _0_id_assign);
	ListValue* _0_str_list = ast.create_assign_list(8, _0_str_assign);
	ListValue* _0_5_id_list = ast.create_assign_list(7, _0_5_id_assign);
	ListValue* _0_5_str_list = ast.create_assign_list(7, _0_5_str_assign);
	ListValue* _1_id_list = ast.create_assign_list(6, _1_id_assign);
	ListValue* _1_str_list = ast.create_assign_list(6, _1_str_assign);
	ListValue* _127_id_list = ast.create_assign_list(5, _127_id_assign);
	ListValue* _127_str_list = ast.create_assign_list(5, _127_str_assign);
	ListValue* _255_id_list = ast.create_assign_list(4, _255_id_assign);
	ListValue* _255_str_list = ast.create_assign_list(4, _255_str_assign);

	CHECK(NodeTools::expect_dictionary(std::bind_front(callback, _0_id_list))(_0_id_list));
	CHECK(NodeTools::expect_dictionary(std::bind_front(callback, _0_str_list))(_0_str_list));
	CHECK(NodeTools::expect_dictionary(std::bind_front(callback, _0_5_id_list))(_0_5_id_list));
	CHECK(NodeTools::expect_dictionary(std::bind_front(callback, _0_5_str_list))(_0_5_str_list));
	CHECK(NodeTools::expect_dictionary(std::bind_front(callback, _1_id_list))(_1_id_list));
	CHECK(NodeTools::expect_dictionary(std::bind_front(callback, _1_str_list))(_1_str_list));
	CHECK(NodeTools::expect_dictionary(std::bind_front(callback, _127_id_list))(_127_id_list));
	CHECK(NodeTools::expect_dictionary(std::bind_front(callback, _127_str_list))(_127_str_list));
	CHECK(NodeTools::expect_dictionary(std::bind_front(callback, _255_id_list))(_255_id_list));
	CHECK(NodeTools::expect_dictionary(std::bind_front(callback, _255_str_list))(_255_str_list));

	static auto length_callback = [](ListValue const* ptr, size_t length) -> size_t {
		auto list = ptr->statements();
		CHECK(ranges::distance(list) == length);
		return length;
	};

	// clang-format off
	CHECK(
		NodeTools::expect_dictionary_and_length(std::bind_front(length_callback, _0_id_list), std::bind_front(callback, _0_id_list))(_0_id_list)
	);
	CHECK(
		NodeTools::expect_dictionary_and_length(std::bind_front(length_callback, _0_str_list), std::bind_front(callback, _0_str_list))(_0_str_list)
	);
	CHECK(
		NodeTools::expect_dictionary_and_length(std::bind_front(length_callback, _0_5_id_list), std::bind_front(callback, _0_5_id_list))(_0_5_id_list)
	);
	CHECK(
		NodeTools::expect_dictionary_and_length(std::bind_front(length_callback, _0_5_str_list), std::bind_front(callback, _0_5_str_list))(_0_5_str_list)
	);
	CHECK(
		NodeTools::expect_dictionary_and_length(std::bind_front(length_callback, _1_id_list), std::bind_front(callback, _1_id_list))(_1_id_list)
	);
	CHECK(
		NodeTools::expect_dictionary_and_length(std::bind_front(length_callback, _1_str_list), std::bind_front(callback, _1_str_list))(_1_str_list)
	);
	CHECK(
		NodeTools::expect_dictionary_and_length(std::bind_front(length_callback, _127_id_list), std::bind_front(callback, _127_id_list))(_127_id_list)
	);
	CHECK(
		NodeTools::expect_dictionary_and_length(std::bind_front(length_callback, _127_str_list), std::bind_front(callback, _127_str_list))(_127_str_list)
	);
	CHECK(
		NodeTools::expect_dictionary_and_length(std::bind_front(length_callback, _255_id_list), std::bind_front(callback, _255_id_list))(_255_id_list)
	);
	CHECK(
		NodeTools::expect_dictionary_and_length(std::bind_front(length_callback, _255_str_list), std::bind_front(callback, _255_str_list))(_255_str_list)
	);
	// clang-format on
}

TEST_CASE(
	"NodeTools expect mapped string functions",
	"[NodeTools][NodeTools-expect-functions][NodeTools-expect-mapped-string-functions]"
) {
	Ast ast;

	static const string_map_t<std::string_view> map //
		{ //
		  { "key_test1", "value_test1"sv },
		  { "key_test2", "value_test2"sv },
		  { "key_test3", "value_test3"sv }
		};

	static auto callback = [](string_map_t<std::string_view> const& map, size_t expected_index, std::string_view val) {
		CHECK_IF(map.size() > expected_index);
		else {
			return false;
		}

		for (auto [index, pair] : map | ranges::views::enumerate) {
			if (index != expected_index) {
				continue;
			}

			CAPTURE(pair.first);
			CHECK_RETURN_BOOL(pair.second == val);
		}

		CHECK_RETURN_BOOL(false);
	};

	CHECK(NodeTools::expect_mapped_string(map, std::bind_front(callback, map, 0))(map.values_container()[0].first));
	CHECK(NodeTools::expect_mapped_string(map, std::bind_front(callback, map, 1))(map.values_container()[1].first));
	CHECK(NodeTools::expect_mapped_string(map, std::bind_front(callback, map, 2))(map.values_container()[2].first));

	static auto callback_false = [](string_map_t<std::string_view> const& map, size_t check_index, std::string_view val) {
		for (auto [index, pair] : map | ranges::views::enumerate) {
			if (index != check_index) {
				continue;
			}

			CAPTURE(pair.first);
			CHECK_FALSE_RETURN_BOOL(pair.second == val);
		}

		CHECK_FALSE_RETURN_BOOL(false);
	};

	CHECK_FALSE(NodeTools::expect_mapped_string(map, std::bind_front(callback_false, map, 1))(map.values_container()[0].first));
	CHECK_FALSE(NodeTools::expect_mapped_string(map, std::bind_front(callback_false, map, 2))(map.values_container()[1].first));
	CHECK_FALSE(NodeTools::expect_mapped_string(map, std::bind_front(callback_false, map, 0))(map.values_container()[2].first));
	CHECK_FALSE(NodeTools::expect_mapped_string(map, std::bind_front(callback_false, map, 3))(map.values_container()[2].first));
}
