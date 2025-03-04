#include "openvic-simulation/dataloader/NodeTools.hpp"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <string_view>
#include <system_error>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <range/v3/iterator/operations.hpp>
#include <range/v3/view/enumerate.hpp>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

#include "../types/Colour.hpp" // IWYU pragma: keep
#include "Helper.hpp" // IWYU pragma: keep
#include "dryad/node.hpp"
#include "dryad/node_map.hpp"
#include "dryad/tree.hpp"
#include "types/Numeric.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::ast;
using namespace std::string_view_literals;

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
	CHECK(!NodeTools::key_value_invalid_callback(""sv, null_value));
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
		CHECK(sv == "expect"sv);
		return true;
	};

	CHECK(NodeTools::expect_identifier(string_view_callback)(id));
	CHECK(!NodeTools::expect_string(string_view_callback)(id));

	CHECK(!NodeTools::expect_identifier(string_view_callback)(str));
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
	CHECK(!NodeTools::expect_bool(bool_callback)(yes_str));
	CHECK(!NodeTools::expect_bool(bool_callback)(no_id));
	CHECK(!NodeTools::expect_bool(bool_callback)(no_str));

	CHECK(!NodeTools::expect_bool(bool_callback)(_2_id));
	CHECK(!NodeTools::expect_bool(bool_callback)(_2_str));
	CHECK(!NodeTools::expect_bool(bool_callback)(_1_id));
	CHECK(!NodeTools::expect_bool(bool_callback)(_1_str));
	CHECK(!NodeTools::expect_bool(bool_callback)(_0_id));
	CHECK(!NodeTools::expect_bool(bool_callback)(_0_str));

	CHECK(!NodeTools::expect_int_bool(bool_callback)(yes_id));
	CHECK(!NodeTools::expect_int_bool(bool_callback)(yes_str));
	CHECK(!NodeTools::expect_int_bool(bool_callback)(no_id));
	CHECK(!NodeTools::expect_int_bool(bool_callback)(no_str));

	CHECK(NodeTools::expect_int_bool(bool_callback)(_2_id));
	CHECK(!NodeTools::expect_int_bool(bool_callback)(_2_str));
	CHECK(NodeTools::expect_int_bool(bool_callback)(_1_id));
	CHECK(!NodeTools::expect_int_bool(bool_callback)(_1_str));
	CHECK(!NodeTools::expect_int_bool(bool_callback)(_0_id));
	CHECK(!NodeTools::expect_int_bool(bool_callback)(_0_str));
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
		if (result.ec != std::errc {}) {
			return false;
		}

		CHECK(check == val);
		return check == val;
	};

	CHECK(NodeTools::expect_int64(std::bind_front(callback, _2_id))(_2_id));
	CHECK(!NodeTools::expect_int64(std::bind_front(callback, _2_str))(_2_str));
	CHECK(NodeTools::expect_int64(std::bind_front(callback, _1_id))(_1_id));
	CHECK(!NodeTools::expect_int64(std::bind_front(callback, _1_str))(_1_str));
	CHECK(NodeTools::expect_int64(std::bind_front(callback, _0_id))(_0_id));
	CHECK(!NodeTools::expect_int64(std::bind_front(callback, _0_str))(_0_str));
	CHECK(NodeTools::expect_int64(std::bind_front(callback, _2002_id))(_2002_id));
	CHECK(!NodeTools::expect_int64(std::bind_front(callback, _2002_str))(_2002_str));
	CHECK(NodeTools::expect_int64(std::bind_front(callback, _1001_id))(_1001_id));
	CHECK(!NodeTools::expect_int64(std::bind_front(callback, _1001_str))(_1001_str));

	CHECK(NodeTools::expect_int64(std::bind_front(callback, _max_long_id))(_max_long_id));
	CHECK(!NodeTools::expect_int64(std::bind_front(callback, _max_long_str))(_max_long_str));
	CHECK(!NodeTools::expect_int64(std::bind_front(callback, _max_ulong_id))(_max_ulong_id));
	CHECK(!NodeTools::expect_int64(std::bind_front(callback, _max_ulong_str))(_max_ulong_str));
	CHECK(NodeTools::expect_int64(std::bind_front(callback, _min_long_id))(_min_long_id));
	CHECK(!NodeTools::expect_int64(std::bind_front(callback, _min_long_str))(_min_long_str));

	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _2_id))(_2_id));
	CHECK(!NodeTools::expect_uint64(std::bind_front(callback, _2_str))(_2_str));
	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _1_id))(_1_id));
	CHECK(!NodeTools::expect_uint64(std::bind_front(callback, _1_str))(_1_str));
	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _0_id))(_0_id));
	CHECK(!NodeTools::expect_uint64(std::bind_front(callback, _0_str))(_0_str));
	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _2002_id))(_2002_id));
	CHECK(!NodeTools::expect_uint64(std::bind_front(callback, _2002_str))(_2002_str));
	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _1001_id))(_1001_id));
	CHECK(!NodeTools::expect_uint64(std::bind_front(callback, _1001_str))(_1001_str));

	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _max_long_id))(_max_long_id));
	CHECK(!NodeTools::expect_uint64(std::bind_front(callback, _max_long_str))(_max_long_str));
	CHECK(NodeTools::expect_uint64(std::bind_front(callback, _max_ulong_id))(_max_ulong_id));
	CHECK(!NodeTools::expect_uint64(std::bind_front(callback, _max_ulong_str))(_max_ulong_str));
	CHECK(!NodeTools::expect_uint64(std::bind_front(callback, _min_long_id))(_min_long_id));
	CHECK(!NodeTools::expect_uint64(std::bind_front(callback, _min_long_str))(_min_long_str));

	auto base_callback = [](FlatValue const* ptr, int base, auto val) -> bool {
		std::string_view sv = ptr->value().view();

		decltype(val) check;
		std::from_chars_result result = OpenVic::StringUtils::string_to_uint64(sv, check, base);
		if (result.ec != std::errc {}) {
			return false;
		}

		CHECK(check == val);
		return check == val;
	};

	auto* _binary_id = ast.create_with_intern<IdentifierValue>("0110"sv);
	auto* _binary_str = ast.create_with_intern<StringValue>("0110"sv);

	CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _binary_id, 2), 2)(_binary_id));
	CHECK(!NodeTools::expect_uint64(std::bind_front(base_callback, _binary_str, 2), 2)(_binary_str));

	auto* _octal_id = ast.create_with_intern<IdentifierValue>("2674"sv);
	auto* _octal_str = ast.create_with_intern<StringValue>("2674"sv);

	CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _octal_id, 8), 8)(_octal_id));
	CHECK(!NodeTools::expect_uint64(std::bind_front(base_callback, _octal_str, 8), 8)(_octal_str));

	SECTION("Detect Octal Base") {
		auto* _detect_octal_id = ast.create_with_intern<IdentifierValue>("02674"sv);
		auto* _detect_octal_str = ast.create_with_intern<StringValue>("02674"sv);

		CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _detect_octal_id, 0), 0)(_detect_octal_id));
		CHECK(!NodeTools::expect_uint64(std::bind_front(base_callback, _detect_octal_str, 0), 0)(_detect_octal_str));

		CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _detect_octal_id, 8), 0)(_detect_octal_id));
		CHECK(!NodeTools::expect_uint64(std::bind_front(base_callback, _detect_octal_str, 8), 0)(_detect_octal_str));
	}

	auto* _hex_id = ast.create_with_intern<IdentifierValue>("0xff2e5"sv);
	auto* _hex_str = ast.create_with_intern<StringValue>("0xff2e5"sv);

	CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _hex_id, 16), 16)(_hex_id));
	CHECK(!NodeTools::expect_uint64(std::bind_front(base_callback, _hex_str, 16), 16)(_hex_str));

	SECTION("Detect Hexadecimal Base") {
		CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _hex_id, 0), 0)(_hex_id));
		CHECK(!NodeTools::expect_uint64(std::bind_front(base_callback, _hex_str, 0), 0)(_hex_str));

		CHECK(NodeTools::expect_uint64(std::bind_front(base_callback, _hex_id, 16), 0)(_hex_id));
		CHECK(!NodeTools::expect_uint64(std::bind_front(base_callback, _hex_str, 16), 0)(_hex_str));
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

		fixed_point_t check = fixed_point_t::_0();
		std::from_chars_result result = check.from_chars(sv.data(), sv.data() + sv.size());
		if (result.ec != std::errc {}) {
			return false;
		}

		CHECK(check == val);
		return check == val;
	};

	CHECK(NodeTools::expect_fixed_point(std::bind_front(callback, _0_01_id))(_0_01_id));
	CHECK(!NodeTools::expect_fixed_point(std::bind_front(callback, _0_01_str))(_0_01_str));
	CHECK(NodeTools::expect_fixed_point(std::bind_front(callback, _2_21_id))(_2_21_id));
	CHECK(!NodeTools::expect_fixed_point(std::bind_front(callback, _2_21_str))(_2_21_str));
	CHECK(NodeTools::expect_fixed_point(std::bind_front(callback, _303_id))(_303_id));
	CHECK(!NodeTools::expect_fixed_point(std::bind_front(callback, _303_str))(_303_str));

	CHECK(!NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _0_01_id)))(_0_01_id));
	CHECK(NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _0_01_str)))(_0_01_str));
	CHECK(!NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _2_21_id)))(_2_21_id));
	CHECK(NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _2_21_str)))(_2_21_str));
	CHECK(!NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _303_id)))(_303_id));
	CHECK(NodeTools::expect_string(NodeTools::expect_fixed_point_str(std::bind_front(callback, _303_str)))(_303_str));
}

TEST_CASE("NodeTools expect colour functions", "[NodeTools][NodeTools-expect-functions][NodeTools-expect-colour-functions]") {
	Ast ast;

	auto _0 = [&ast] {
		return ast.create<ValueStatement>(ast.create_with_intern<IdentifierValue>("0"sv));
	};
	auto _0_5 = [&ast] {
		return ast.create<ValueStatement>(ast.create_with_intern<IdentifierValue>("0.5"sv));
	};
	auto _1 = [&ast] {
		return ast.create<ValueStatement>(ast.create_with_intern<IdentifierValue>("1"sv));
	};
	auto _127 = [&ast] {
		return ast.create<ValueStatement>(ast.create_with_intern<IdentifierValue>("127"sv));
	};
	auto _255 = [&ast] {
		return ast.create<ValueStatement>(ast.create_with_intern<IdentifierValue>("255"sv));
	};

	StatementList _0_slist;
	_0_slist.push_back(_0());
	_0_slist.push_back(_0());
	_0_slist.push_back(_0());
	auto* _0_list = ast.create<ListValue>(_0_slist);

	StatementList _0_5_slist;
	_0_5_slist.push_back(_0_5());
	_0_5_slist.push_back(_0_5());
	_0_5_slist.push_back(_0_5());
	auto* _0_5_list = ast.create<ListValue>(_0_5_slist);

	StatementList _1_slist;
	_1_slist.push_back(_1());
	_1_slist.push_back(_1());
	_1_slist.push_back(_1());
	auto* _1_list = ast.create<ListValue>(_1_slist);

	StatementList _127_slist;
	_127_slist.push_back(_127());
	_127_slist.push_back(_127());
	_127_slist.push_back(_127());
	auto* _127_list = ast.create<ListValue>(_127_slist);

	StatementList _255_slist;
	_255_slist.push_back(_255());
	_255_slist.push_back(_255());
	_255_slist.push_back(_255());
	auto* _255_list = ast.create<ListValue>(_255_slist);

	StatementList _mix_slist;
	_mix_slist.push_back(_0());
	_mix_slist.push_back(_0_5());
	_mix_slist.push_back(_255());
	auto* _mix_list = ast.create<ListValue>(_mix_slist);

	auto callback = [](ListValue const* ptr, colour_t val) -> bool {
		auto list = ptr->statements();
		CHECK(ranges::distance(list) == 3);
		if (ranges::distance(list) != 3) {
			return false;
		}

		colour_t check = colour_t::null();
		for (auto [index, sub_node] : list | ranges::views::enumerate) {
			if (auto const* value = dryad::node_try_cast<ValueStatement>(sub_node)) {
				if (auto const* id = dryad::node_try_cast<IdentifierValue>(value->value())) {
					std::string_view sv = id->value().view();
					fixed_point_t fp = fixed_point_t::_0();
					std::from_chars_result result = fp.from_chars(sv.data(), sv.data() + sv.size());
					CHECK(result.ec == std::errc {});
					if (result.ec != std::errc {}) {
						return false;
					}
					if (fp <= 1) {
						fp *= 255;
					}
					check[index] = fp.to_int64_t();
				}
			}
		}

		CHECK(check == val);
		return check == val;
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
		if (result.ec != std::errc {}) {
			return false;
		}

		CHECK(check == val);
		return check == val;
	};

	CHECK(NodeTools::expect_colour_hex(std::bind_front(callback, orange_id))(orange_id));
	CHECK(!NodeTools::expect_colour_hex(std::bind_front(callback, orange_str))(orange_str));
}
