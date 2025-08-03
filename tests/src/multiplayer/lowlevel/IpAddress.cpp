#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"

#include <string_view>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>
#include <snitch/snitch_string.hpp>
#include <snitch/snitch_string_utility.hpp>

using namespace OpenVic;
using namespace std::string_view_literals;

namespace snitch {
	[[nodiscard]] inline static constexpr bool append(snitch::small_string_span ss, IpAddress const& ip) {
		return append(ss, ip.to_array());
	}
}

TEST_CASE("IpAddress Constructors", "[IpAddress][IpAddress-constructor]") {
	static constexpr IpAddress empty;
	CONSTEXPR_CHECK(!empty.is_valid());
	CONSTEXPR_CHECK(!empty.is_wildcard());
	CONSTEXPR_CHECK(!empty.is_ipv4());
	CONSTEXPR_CHECK(empty != IpAddress {});

	static constexpr IpAddress wildcard_cstr = "*";
	CONSTEXPR_CHECK(!wildcard_cstr.is_valid());
	CONSTEXPR_CHECK(wildcard_cstr.is_wildcard());
	CONSTEXPR_CHECK(!wildcard_cstr.is_ipv4());
	CONSTEXPR_CHECK(wildcard_cstr != IpAddress { "*" });

	static constexpr IpAddress wildcard_str = "*"sv;
	CONSTEXPR_CHECK(!wildcard_str.is_valid());
	CONSTEXPR_CHECK(wildcard_str.is_wildcard());
	CONSTEXPR_CHECK(!wildcard_str.is_ipv4());
	CONSTEXPR_CHECK(wildcard_str != IpAddress { "*"sv });

	static constexpr IpAddress ipv4_cstr = "128.127.126.125";
	CONSTEXPR_CHECK(ipv4_cstr.is_valid());
	CONSTEXPR_CHECK(!ipv4_cstr.is_wildcard());
	CONSTEXPR_CHECK(ipv4_cstr.is_ipv4());
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv4()[0] == 128);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv4()[1] == 127);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv4()[2] == 126);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv4()[3] == 125);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[0] == 0);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[1] == 0);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[2] == 0);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[3] == 0);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[4] == 0);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[5] == 0);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[6] == 0);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[7] == 0);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[8] == 0);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[9] == 0);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[10] == 0xff);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[11] == 0xff);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[12] == 128);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[13] == 127);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[14] == 126);
	CONSTEXPR_CHECK(ipv4_cstr.get_ipv6()[15] == 125);
	CONSTEXPR_CHECK(ipv4_cstr == IpAddress { "128.127.126.125" });
	CONSTEXPR_CHECK(ipv4_cstr == IpAddress { "0000:0000:0000:0000:0000:ffff:807f:7e7d" });
	CONSTEXPR_CHECK(ipv4_cstr == IpAddress { 128, 127, 126, 125 });
	CONSTEXPR_CHECK(ipv4_cstr == IpAddress { 0, 0, 0x0000ffff, 0x807f7e7d, true });

	static constexpr IpAddress ipv4_str = "125.126.127.128"sv;
	CONSTEXPR_CHECK(ipv4_str.is_valid());
	CONSTEXPR_CHECK(!ipv4_str.is_wildcard());
	CONSTEXPR_CHECK(ipv4_str.is_ipv4());
	CONSTEXPR_CHECK(ipv4_str.get_ipv4()[0] == 125);
	CONSTEXPR_CHECK(ipv4_str.get_ipv4()[1] == 126);
	CONSTEXPR_CHECK(ipv4_str.get_ipv4()[2] == 127);
	CONSTEXPR_CHECK(ipv4_str.get_ipv4()[3] == 128);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[0] == 0);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[1] == 0);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[2] == 0);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[3] == 0);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[4] == 0);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[5] == 0);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[6] == 0);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[7] == 0);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[8] == 0);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[9] == 0);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[10] == 0xff);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[11] == 0xff);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[12] == 125);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[13] == 126);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[14] == 127);
	CONSTEXPR_CHECK(ipv4_str.get_ipv6()[15] == 128);
	CONSTEXPR_CHECK(ipv4_str == IpAddress { "125.126.127.128"sv });
	CONSTEXPR_CHECK(ipv4_str == IpAddress { "0000:0000:0000:0000:0000:ffff:7d7e:7f80"sv });
	CONSTEXPR_CHECK(ipv4_str == IpAddress { 125, 126, 127, 128 });
	CONSTEXPR_CHECK(ipv4_str == IpAddress { 0, 0, 0x0000ffff, 0x7d7e7f80, true });

	static constexpr IpAddress ipv6_cstr = "0000:0000:5500:0000:0000:ffff:8190:3238";
	CONSTEXPR_CHECK(ipv6_cstr.is_valid());
	CONSTEXPR_CHECK(!ipv6_cstr.is_wildcard());
	CONSTEXPR_CHECK(!ipv6_cstr.is_ipv4());
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[0] == 0);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[1] == 0);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[2] == 0);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[3] == 0);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[4] == 0x55);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[5] == 0);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[6] == 0);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[7] == 0);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[8] == 0);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[9] == 0);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[10] == 0xff);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[11] == 0xff);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[12] == 0x81);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[13] == 0x90);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[14] == 0x32);
	CONSTEXPR_CHECK(ipv6_cstr.get_ipv6()[15] == 0x38);
	CONSTEXPR_CHECK(ipv6_cstr == IpAddress { "0000:0000:5500:0000:0000:ffff:8190:3238" });
	CONSTEXPR_CHECK(ipv6_cstr == IpAddress { 0, 0x55000000, 0x0000ffff, 0x81903238, true });

	static constexpr IpAddress ipv6_str = "24f1:0000:0000:0000:0000:ffff:8190:3238"sv;
	CONSTEXPR_CHECK(ipv6_str.is_valid());
	CONSTEXPR_CHECK(!ipv6_str.is_wildcard());
	CONSTEXPR_CHECK(!ipv6_str.is_ipv4());
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[0] == 0x24);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[1] == 0xf1);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[2] == 0);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[3] == 0);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[4] == 0);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[5] == 0);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[6] == 0);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[7] == 0);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[8] == 0);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[9] == 0);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[10] == 0xff);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[11] == 0xff);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[12] == 0x81);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[13] == 0x90);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[14] == 0x32);
	CONSTEXPR_CHECK(ipv6_str.get_ipv6()[15] == 0x38);
	CONSTEXPR_CHECK(ipv6_str == IpAddress { "24f1:0000:0000:0000:0000:ffff:8190:3238"sv });
	CONSTEXPR_CHECK(ipv6_str == IpAddress { 0x24f10000, 0, 0x0000ffff, 0x81903238, true });
}

TEST_CASE("IpAddress setter methods", "[IpAddress][IpAddress-setters]") {
	static constexpr auto ipv4_setter = [](IpAddress&& addr) constexpr {
		addr.set_ipv4({ 55, 23, 5, 1 });
		return addr;
	};
	static constexpr IpAddress ipv4_set = ipv4_setter({});
	CONSTEXPR_CHECK(ipv4_set.is_valid());
	CONSTEXPR_CHECK(!ipv4_set.is_wildcard());
	CONSTEXPR_CHECK(ipv4_set.is_ipv4());
	CONSTEXPR_CHECK(ipv4_set.get_ipv4()[0] == 55);
	CONSTEXPR_CHECK(ipv4_set.get_ipv4()[1] == 23);
	CONSTEXPR_CHECK(ipv4_set.get_ipv4()[2] == 5);
	CONSTEXPR_CHECK(ipv4_set.get_ipv4()[3] == 1);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[0] == 0);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[1] == 0);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[2] == 0);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[3] == 0);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[4] == 0);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[5] == 0);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[6] == 0);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[7] == 0);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[8] == 0);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[9] == 0);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[10] == 0xff);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[11] == 0xff);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[12] == 55);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[13] == 23);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[14] == 5);
	CONSTEXPR_CHECK(ipv4_set.get_ipv6()[15] == 1);
	CONSTEXPR_CHECK(ipv4_set == IpAddress { "55.23.5.1"sv });
	CONSTEXPR_CHECK(ipv4_set == IpAddress { "0000:0000:0000:0000:0000:ffff:3717:0501"sv });
	CONSTEXPR_CHECK(ipv4_set == IpAddress { 55, 23, 5, 1 });
	CONSTEXPR_CHECK(ipv4_set == IpAddress { 0, 0, 0x0000ffff, 0x37170501, true });

	static constexpr auto ipv6_setter = [](IpAddress&& addr) constexpr {
		addr.set_ipv6({ 0xab, 0x56, 0, 0x35, 0, 0, 0, 0x22, 0, 0x32, 0, 0x01, 0, 0, 0, 0 });
		return addr;
	};
	static constexpr IpAddress ipv6_set = ipv6_setter({});
	CONSTEXPR_CHECK(ipv6_set.is_valid());
	CONSTEXPR_CHECK(!ipv6_set.is_wildcard());
	CONSTEXPR_CHECK(!ipv6_set.is_ipv4());
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[0] == 0xab);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[1] == 0x56);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[2] == 0);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[3] == 0x35);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[4] == 0);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[5] == 0);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[6] == 0);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[7] == 0x22);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[8] == 0);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[9] == 0x32);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[10] == 0);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[11] == 0x01);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[12] == 0);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[13] == 0);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[14] == 0);
	CONSTEXPR_CHECK(ipv6_set.get_ipv6()[15] == 0);
	CONSTEXPR_CHECK(ipv6_set == IpAddress { "ab56:0035:0000:0022:0032:0001:0000:0000"sv });
	CONSTEXPR_CHECK(ipv6_set == IpAddress { 0xab560035, 0x00000022, 0x00320001, 0, true });
}

TEST_CASE("IpAddress Conversion methods", "[IpAddress][IpAddress-conversion]") {
	static constexpr IpAddress ipv4 = { 26, 72, 182, 254 };
	static constexpr IpAddress ipv6 = { 0xab65002f, 0x002253f3, 0, 0x0321fe00, true };

	CONSTEXPR_CHECK(ipv4.to_array() == "26.72.182.254"sv);
	CONSTEXPR_CHECK(ipv4.to_array(false) == "0:0:0:0:0:ffff:1a48:b6fe"sv);
	CONSTEXPR_CHECK(ipv4.to_array(false, IpAddress::to_chars_option::COMPRESS_IPV6) == "::ffff:1a48:b6fe"sv);
	CONSTEXPR_CHECK(
		ipv4.to_array(false, IpAddress::to_chars_option::EXPAND_IPV6) == "0000:0000:0000:0000:0000:ffff:1a48:b6fe"sv
	);

	CONSTEXPR_CHECK(ipv6.to_array() == "ab65:2f:22:53f3:0:0:321:fe00"sv);
	CONSTEXPR_CHECK(ipv6.to_array(false) == "ab65:2f:22:53f3:0:0:321:fe00"sv);
	CONSTEXPR_CHECK(ipv6.to_array(false, IpAddress::to_chars_option::COMPRESS_IPV6) == "ab65:2f:22:53f3::321:fe00"sv);
	CONSTEXPR_CHECK(
		ipv6.to_array(false, IpAddress::to_chars_option::EXPAND_IPV6) == "ab65:002f:0022:53f3:0000:0000:0321:fe00"sv
	);

	static constexpr IpAddress compress_begin = "0000:0000:0022:53f3:0000:0000:0321:fe00"sv;
	CONSTEXPR_CHECK(compress_begin.to_array(false, IpAddress::to_chars_option::COMPRESS_IPV6) == "::22:53f3:0:0:321:fe00"sv);
	static constexpr IpAddress compress_middle = "0000:0000:0022:53f3:0000:0000:0000:fe00"sv;
	CONSTEXPR_CHECK(compress_middle.to_array(false, IpAddress::to_chars_option::COMPRESS_IPV6) == "0:0:22:53f3::fe00"sv);
}
