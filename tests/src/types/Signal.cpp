#include "openvic-simulation/types/Signal.hpp"

#include <cmath>
#include <memory>
#include <sstream>
#include <string>

#include "openvic-simulation/types/SpinMutex.hpp"

#include "Helper.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace std::string_view_literals;

using SignalTypes =
	snitch::type_list<OpenVic::signal<int&, int>, nothread::signal<int&, int>, basic_signal<spin_mutex, int&, int>>;

void test_func(int& sum, int i) {
	sum += i;
}

void test_func2(int& sum, int i) {
	sum += 2 * i;
}

struct test_struct {
	static void static_func(int& sum, int i) {
		sum += i;
	}
	static void static_func2(int& sum, int i) {
		sum += 2 * i;
	}

	// Warning: Linker on Windows release_template target compresses these together
	// TODO: Add windows linker option /OPT:NOICF?
	void func(int& sum, int i) {
		sum += i;
	}
	void func_c(int& sum, int i) const {
		sum += i;
	}
	void func_v(int& sum, int i) volatile {
		sum += i;
	}
	void func_cv(int& sum, int i) const volatile {
		sum += i;
	}
	//

	void unique_func(int& sum, int i) const {
		sum += i + 1;
	}
};

struct test_callable_overload {
	void operator()(int& sum, int i) {
		sum += i;
	}
	void operator()(int& sum, double i) {
		sum += int(std::round(4 * i));
	}
};

struct test_callable {
	void operator()(int& sum, int i) {
		sum += i;
	}
};
struct test_callable_c {
	void operator()(int& sum, int i) const {
		sum += i;
	}
};
struct test_callable_v {
	void operator()(int& sum, int i) volatile {
		sum += i;
	}
};
struct test_callable_cv {
	void operator()(int& sum, int i) const volatile {
		sum += i;
	}
};

TEMPLATE_LIST_TEST_CASE("signal slot_count", "[signal][signal-slot_count]", SignalTypes) {
	TestType signal;
	test_struct p;

	signal.connect(&test_struct::func, &p);
	CHECK(signal.slot_count() == 1);
	signal.connect(&test_struct::func_c, &p);
	CHECK(signal.slot_count() == 2);
	signal.connect(&test_struct::func_v, &p);
	CHECK(signal.slot_count() == 3);
	signal.connect(&test_struct::func_cv, &p);
	CHECK(signal.slot_count() == 4);

	{
		scoped_connection conn = signal.connect(&test_struct::func_cv, &p);
		CHECK(signal.slot_count() == 5);
	}
	CHECK(signal.slot_count() == 4);

	connection conn = signal.connect(&test_struct::func_v, &p);
	CHECK(signal.slot_count() == 5);
	conn.disconnect();
	CHECK(signal.slot_count() == 4);

	signal.disconnect_all();
	CHECK(signal.slot_count() == 0);
}

TEMPLATE_LIST_TEST_CASE("signal Connect methods", "[signal][signal-connect]", SignalTypes) {
	TestType signal;
	int sum = 0;

	SECTION("free functions") {
		connection c1 = signal.connect(test_func);
		signal(sum, 1);
		CHECK(sum == 1);

		signal.connect(test_func2);
		connect(signal, test_func);
		signal(sum, 1);
		CHECK(sum == 5);
	}

	SECTION("static functions") {
		signal.connect(&test_struct::static_func);
		signal(sum, 1);
		CHECK(sum == 1);

		signal.connect(&test_struct::static_func2);
		connect(signal, &test_struct::static_func);
		signal(sum, 1);
		CHECK(sum == 5);
	}

	SECTION("member function pointer") {
		test_struct p;

		signal.connect(&test_struct::func, &p);
		signal.connect(&test_struct::func_c, &p);
		signal.connect(&test_struct::func_v, &p);
		signal.connect(&test_struct::func_cv, &p);
		connect(signal, &test_struct::func, &p);

		signal(sum, 1);
		CHECK(sum == 5);
	}

	SECTION("functor class") {
		signal.connect(test_callable {});
		signal.connect(test_callable_c {});
		signal.connect(test_callable_v {});
		signal.connect(test_callable_cv {});
		connect(signal, test_callable {});

		signal(sum, 1);
		CHECK(sum == 5);
	}

	SECTION("overloaded functor class") {
		OpenVic::signal<int&, double> double_signal;

		signal.connect(test_callable_overload {});
		connect(signal, test_callable_overload {});
		signal(sum, 1);
		CHECK(sum == 2);

		double_signal.connect(test_callable_overload {});
		connect(double_signal, test_callable_overload {});
		double_signal(sum, 1);
		CHECK(sum == 10);
	}

	SECTION("") {
		signal.connect([](int& sum, int i) {
			sum += i;
		});
		connect(signal, [](int& sum, int i) {
			sum += i;
		});
		signal(sum, 1);
		CHECK(sum == 2);

		signal.connect([](int& sum, int i) mutable {
			sum += 2 * i;
		});
		connect(signal, [](int& sum, int i) mutable {
			sum += 2 * i;
		});
		signal(sum, 1);
		CHECK(sum == 8);
	}
}

TEST_CASE("signal generic lambda Connect methods", "[signal][signal-connect]") {
	std::stringstream s;

	auto f = [&](auto a, auto... args) {
		using result_t = int[];
		s << a;
		result_t r {
			1,
			((void)(s << args), 1)...,
		};
		(void)r;
	};

	OpenVic::signal<int> signal1;
	OpenVic::signal<std::string> signal2;
	OpenVic::signal<double> signal3;

	signal1.connect(f);
	signal2.connect(f);
	signal3.connect(f);
	connect(signal1, f);
	connect(signal2, f);
	connect(signal3, f);
	signal1(1);
	signal2("foo");
	signal3(4.1);

	CHECK(s.str() == "11foofoo4.14.1");
}

TEMPLATE_LIST_TEST_CASE("signal Emit methods", "[signal][signal-emit]", SignalTypes) {
	TestType signal;
	int sum = 0;

	SECTION("nonmutable") {
		connection c1 = signal.connect(test_func);
		int v = 1;
		signal(sum, v);
		CHECK(sum == 1);

		signal.connect(test_func2);
		signal(sum, v);
		CHECK(sum == 4);
	}

	SECTION("mutable") {
		signal.connect([](int& r, int = 0) {
			r += 1;
		});
		signal(sum, 0);
		CHECK(sum == 1);

		signal.connect([](int& r, int = 0) mutable {
			r += 2;
		});
		signal(sum, 0);
		CHECK(sum == 4);
	}
}

// void test_compatible_args() {
TEST_CASE("signal Emit methods argument conversion", "[signal][signal-emit][signal-emit-argument_conversion]") {
	long ll = 0;
	std::string ss;
	short ii = 0;

	auto f = [&](long l, const std::string& s, short i) {
		ll = l;
		ss = s;
		ii = i;
	};

	OpenVic::signal<int, std::string, bool> signal;
	signal.connect(f);
	signal('0', "foo", true);

	CHECK(ll == 48);
	CHECK(ss == "foo");
	CHECK(ii == 1);
}

void test_compatible_args_chaining() {
	long ll = 0;
	std::string ss;
	short ii = 0;

	auto f = [&](long l, const std::string& s, short i) {
		ll = l;
		ss = s;
		ii = i;
	};

	OpenVic::signal<long, std::string, short> sig1;
	sig1.connect(f);

	OpenVic::signal<int, std::string, bool> sig2;
	connect(sig2, sig1);
	sig2('0', "foo", true);

	CHECK(ll == 48);
	CHECK(ss == "foo");
	CHECK(ii == 1);
}

TEMPLATE_LIST_TEST_CASE("signal Disconnect methods", "[signal][signal-disconnect]", SignalTypes) {
	TestType signal;
	int sum = 0;

	SECTION("basic") {
		SECTION("only connected") {
			connection sc = signal.connect(test_func);
			signal(sum, 1);
			CHECK(sum == 1);

			sc.disconnect();
			signal(sum, 1);
			CHECK(sum == 1);
			CHECK(!sc.valid());
		}

		SECTION("first connected") {
			connection sc = signal.connect(test_func);
			signal(sum, 1);
			CHECK(sum == 1);

			signal.connect(test_func2);
			signal(sum, 1);
			CHECK(sum == 4);

			sc.disconnect();
			signal(sum, 1);
			CHECK(sum == 6);
			CHECK(!sc.valid());
		}

		SECTION("last connected") {
			signal.connect(test_func);
			signal(sum, 1);
			CHECK(sum == 1);

			connection sc = signal.connect(test_func2);
			signal(sum, 1);
			CHECK(sum == 4);

			sc.disconnect();
			signal(sum, 1);
			CHECK(sum == 5);
			CHECK(!sc.valid());
		}
	}

	SECTION("callable") {
		SECTION("function pointer") {
			signal.connect(test_func);
			signal.connect(test_func2);
			signal.connect(test_func2);
			signal(sum, 1);
			CHECK(sum == 5);
			size_t c = signal.disconnect(&test_func2);
			CHECK(c == 2);
			signal(sum, 1);
			CHECK(sum == 6);
		}

		SECTION("function") {
			signal.connect(test_func);
			signal.connect(test_func2);
			signal(sum, 1);
			CHECK(sum == 3);
			signal.disconnect(test_func);
			signal(sum, 1);
			CHECK(sum == 5);
		}
	}

	SECTION("object") {
		SECTION("pointer") {
			test_struct p1, p2;

			signal.connect(&test_struct::func, &p1);
			signal.connect(&test_struct::func_c, &p2);
			signal(sum, 1);
			CHECK(sum == 2);
			signal.disconnect(&p1);
			signal(sum, 1);
			CHECK(sum == 3);
		}

		SECTION("shared_ptr") {
			auto p1 = std::make_shared<test_struct>();
			test_struct p2;

			signal.connect(&test_struct::func, p1);
			signal.connect(&test_struct::func_c, &p2);
			signal(sum, 1);
			CHECK(sum == 2);
			signal.disconnect(p1);
			signal(sum, 1);
			CHECK(sum == 3);
		}
	}

	SECTION("object with member function") {
		SECTION("pointer") {
			test_struct p1, p2;

			signal.connect(&test_struct::func, &p1);
			signal.connect(&test_struct::func, &p2);
			// Warning: func_c, func_v, and func_cv are equal to func on Windows release_template
			// TODO: Add windows linker option /OPT:NOICF?
			signal.connect(&test_struct::unique_func, &p1);
			signal.connect(&test_struct::unique_func, &p2);
			//
			signal(sum, 1);
			CHECK(sum == 6);
			CHECK(signal.disconnect(&test_struct::unique_func, &p2) == 1);
			signal(sum, 1);
			CHECK(sum == 10);
		}

		SECTION("shared_ptr") {
			std::shared_ptr<test_struct> p1 = std::make_shared<test_struct>();
			std::shared_ptr<test_struct> p2 = std::make_shared<test_struct>();

			signal.connect(&test_struct::func, p1);
			signal.connect(&test_struct::func, p2);
			// Warning: func_c, func_v, and func_cv are equal to func on Windows release_template
			// TODO: Add windows linker option /OPT:NOICF?
			signal.connect(&test_struct::unique_func, p1);
			signal.connect(&test_struct::unique_func, p2);
			//
			signal(sum, 1);
			CHECK(sum == 6);
			CHECK(signal.disconnect(&test_struct::unique_func, p2) == 1);
			signal(sum, 1);
			CHECK(sum == 10);
		}

		SECTION("tracker") {
			std::shared_ptr<bool> t = std::make_shared<bool>();
			signal.connect(test_func);
			signal.connect(test_func2);
			signal.connect(test_func, t);
			signal.connect(test_func2, t);
			signal(sum, 1);
			CHECK(sum == 6);
			signal.disconnect(test_func2, t);
			signal(sum, 1);
			CHECK(sum == 10);
		}
	}

	SECTION("disconnct_all") {
		signal.connect(test_func);
		signal.connect(test_func2);
		signal(sum, 1);
		CHECK(sum == 3);

		signal.disconnect_all();
		signal(sum, 1);
		CHECK(sum == 3);
	}
}

TEMPLATE_LIST_TEST_CASE("signal scoped_connection", "[signal][signal-scoped_connection]", SignalTypes) {
	TestType signal;
	int sum = 0;

	SECTION("explicit") {
		{
			scoped_connection sc1 = signal.connect_scoped(test_func);
			signal(sum, 1);
			CHECK(sum == 1);

			scoped_connection sc2 = signal.connect_scoped(test_func2);
			signal(sum, 1);
			CHECK(sum == 4);
		}

		signal(sum, 1);
		CHECK(sum == 4);
	}

	SECTION("implicit") {
		{
			scoped_connection sc1 = signal.connect(test_func);
			signal(sum, 1);
			CHECK(sum == 1);

			auto sc2 = signal.connect_scoped(test_func2);
			signal(sum, 1);
			CHECK(sum == 4);
		}

		signal(sum, 1);
		CHECK(sum == 4);
	}
}

TEMPLATE_LIST_TEST_CASE("signal Block methods", "[signal][signal-block]", SignalTypes) {
	TestType signal;
	int sum = 0;

	SECTION("connection") {
		connection c1 = signal.connect(test_func);
		signal.connect(test_func2);
		signal(sum, 1);
		CHECK(sum == 3);

		c1.block();
		signal(sum, 1);
		CHECK(sum == 5);

		c1.unblock();
		signal(sum, 1);
		CHECK(sum == 8);
	}

	SECTION("connection_blocker") {
		connection c1 = signal.connect(test_func);
		signal.connect(test_func2);
		signal(sum, 1);
		CHECK(sum == 3);

		{
			connection_blocker cb = c1.blocker();
			signal(sum, 1);
			CHECK(sum == 5);
		}

		signal(sum, 1);
		CHECK(sum == 8);
	}

	SECTION("signal") {
		signal.connect(test_func);
		signal.connect(test_func2);
		signal(sum, 1);
		CHECK(sum == 3);

		signal.block();
		signal(sum, 1);
		CHECK(sum == 3);

		signal.unblock();
		signal(sum, 1);
		CHECK(sum == 6);
	}
}

TEMPLATE_LIST_TEST_CASE("signal Assignment operators", "[signal][signal-assignment-operators]", SignalTypes) {
	TestType signal;
	int sum = 0;

	SECTION("connection") {
		connection sc1 = signal.connect(test_func);
		connection sc2 = signal.connect(test_func2);

		connection sc3 = sc1;
		connection sc4 { sc2 };

		connection sc5 = std::move(sc3);
		connection sc6 { std::move(sc4) };

		signal(sum, 1);
		CHECK(sum == 3);

		sc5.block();
		signal(sum, 1);
		CHECK(sum == 5);

		sc1.unblock();
		signal(sum, 1);
		CHECK(sum == 8);

		sc6.disconnect();
		signal(sum, 1);
		CHECK(sum == 9);
	}

	SECTION("scoped_connection") {
		{
			auto sc1 = signal.connect_scoped(test_func);
			signal(sum, 1);
			CHECK(sum == 1);

			auto sc2 = signal.connect_scoped(test_func2);
			signal(sum, 1);
			CHECK(sum == 4);

			auto sc3 = std::move(sc1);
			signal(sum, 1);
			CHECK(sum == 7);

			auto sc4 { std::move(sc2) };
			signal(sum, 1);
			CHECK(sum == 10);
		}

		signal(sum, 1);
		CHECK(sum == 10);
	}

	SECTION("signal") {
		signal.connect(test_func);
		signal.connect(test_func2);

		signal(sum, 1);
		CHECK(sum == 3);

		TestType signal2 = std::move(signal);
		signal2(sum, 1);
		CHECK(sum == 6);

		TestType signal3 = std::move(signal2);
		signal3(sum, 1);
		CHECK(sum == 9);
	}
}

template<typename T>
struct test_object_loop {
	test_object_loop();
	test_object_loop(T i) : v { i } {}

	const T& val() const {
		return v;
	}
	T& val() {
		return v;
	}
	void set_val(const T& i) {
		if (i != v) {
			v = i;
			s(i);
		}
	}

	OpenVic::signal<T>& get_signal() {
		return s;
	}

private:
	T v;
	OpenVic::signal<T> s;
};

TEST_CASE("signal loop", "[signal][signal-loop]") {
	test_object_loop<int> i1(0);
	test_object_loop<int> i2(3);

	i1.get_signal().connect(&test_object_loop<int>::set_val, &i2);
	i2.get_signal().connect(&test_object_loop<int>::set_val, &i1);

	i1.set_val(1);

	CHECK(i1.val() == 1);
	CHECK(i2.val() == 1);
}

TEST_CASE("signal multiple emits", "[signal][signal-multiple-emits]") {
	std::stringstream s;

	OpenVic::signal<std::string> signal;
	connection conn = signal.connect([&s](auto a, auto... args) {
		using result_t = int[];
		s << a;
		result_t r {
			1,
			((void)(s << args), 1)...,
		};
		(void)r;
	});
	signal("Hello");
	signal(" world!");

	CHECK(s.str() == "Hello world!");
}
