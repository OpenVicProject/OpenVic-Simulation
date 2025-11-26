#include "openvic-simulation/core/error/ErrorSet.hpp"

#include <string_view>

#include "openvic-simulation/core/error/Error.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("ErrorSet Constructor methods", "[ErrorSet][ErrorSet-constructor]") {
	CHECK(ErrorSet {}.none());
	CHECK(ErrorSet { Error::OK }.none());
	CHECK(ErrorSet { Error::FAILED }.any());

	ErrorSet set_1;
	ErrorSet set_2 = Error::OK;
	ErrorSet set_3 = Error::BUG;

	CHECK(set_1.none());
	CHECK(set_2.none());
	CHECK(set_3.any());
	CHECK(set_3[Error::BUG]);
	CHECK_FALSE(set_3[Error::FAILED]);

	ErrorSet set_4 = set_3;

	CHECK(set_4.any());
	CHECK(set_4[Error::BUG]);
	CHECK_FALSE(set_4[Error::FAILED]);

	ErrorSet set_5 = std::move(set_4);

	CHECK(set_5.any());
	CHECK(set_5[Error::BUG]);
	CHECK_FALSE(set_5[Error::FAILED]);
}

TEST_CASE("ErrorSet Operators", "[ErrorSet][ErrorSet-operators]") {
	ErrorSet set_ok = Error::OK;
	ErrorSet set_bug = Error::BUG;
	ErrorSet set_multiple = Error::BUG;
	set_multiple |= Error::FAILED;

	CHECK(set_ok[Error::OK]);
	CHECK_FALSE(set_ok[Error::BUG]);
	CHECK_FALSE(set_ok[Error::FAILED]);
	CHECK_FALSE(set_ok[Error::ALREADY_EXISTS]);
	CHECK(set_ok == Error::OK);
	CHECK_FALSE(set_ok == Error::BUG);
	CHECK_FALSE(set_ok == Error::FAILED);
	CHECK_FALSE(set_ok == Error::ALREADY_EXISTS);
	CHECK_FALSE(set_ok == set_bug);
	CHECK_FALSE(set_ok == set_multiple);
	CHECK_FALSE(set_ok[set_bug]);
	CHECK_FALSE(set_ok[set_multiple]);

	CHECK_FALSE(set_bug[Error::OK]);
	CHECK(set_bug[Error::BUG]);
	CHECK_FALSE(set_bug[Error::FAILED]);
	CHECK_FALSE(set_bug[Error::ALREADY_EXISTS]);
	CHECK_FALSE(set_bug == Error::OK);
	CHECK(set_bug == Error::BUG);
	CHECK_FALSE(set_bug == Error::FAILED);
	CHECK_FALSE(set_bug == Error::ALREADY_EXISTS);
	CHECK_FALSE(set_bug == set_ok);
	CHECK_FALSE(set_bug == set_multiple);
	CHECK_FALSE(set_bug[set_ok]);
	CHECK_FALSE(set_bug[set_multiple]);

	CHECK_FALSE(set_multiple[Error::OK]);
	CHECK(set_multiple[Error::BUG]);
	CHECK(set_multiple[Error::FAILED]);
	CHECK_FALSE(set_multiple[Error::ALREADY_EXISTS]);
	CHECK_FALSE(set_multiple == Error::OK);
	CHECK(set_multiple == Error::BUG);
	CHECK(set_multiple == Error::FAILED);
	CHECK_FALSE(set_multiple == Error::ALREADY_EXISTS);
	CHECK_FALSE(set_multiple == set_bug);
	CHECK_FALSE(set_multiple == set_ok);
	CHECK(set_multiple[set_bug]);
	CHECK_FALSE(set_multiple[set_ok]);

	CHECK((set_multiple | Error::ALREADY_EXISTS)[Error::ALREADY_EXISTS]);
	CHECK_FALSE((set_multiple | Error::ALREADY_EXISTS)[Error::CONNECTION_ERROR]);

	set_multiple |= Error::CONNECTION_ERROR;
	CHECK(set_multiple[Error::BUG]);
	CHECK(set_multiple[Error::FAILED]);
	CHECK(set_multiple[Error::CONNECTION_ERROR]);

	set_multiple |= Error::OK;
	CHECK(set_multiple[Error::BUG]);
	CHECK(set_multiple[Error::FAILED]);
	CHECK(set_multiple[Error::CONNECTION_ERROR]);
}

TEST_CASE("ErrorSet to_string", "[ErrorSet][ErrorSet-to_string]") {
	using namespace std::string_view_literals;

	ErrorSet set_ok = Error::OK;
	ErrorSet set_bug = Error::BUG;
	ErrorSet set_multiple = Error::BUG;
	set_multiple |= Error::FAILED;

	CHECK(set_ok.to_string() == "OK"sv);
	CHECK(set_bug.to_string() == "Bug"sv);
	CHECK(set_multiple.to_string() == "Failed, Bug"sv);

	CHECK(set_ok.to_bitset_string() == "00000000000000000000000"sv);
	CHECK(set_bug.to_bitset_string() == "10000000000000000000000"sv);
	CHECK(set_multiple.to_bitset_string() == "10000000000000000000001"sv);
}
