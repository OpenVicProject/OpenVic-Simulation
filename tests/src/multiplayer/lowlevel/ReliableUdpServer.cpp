#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpServer.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <memory>
#include <string_view>
#include <thread>

#include <zpp_bits.h>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpClient.hpp"

#include "Helper.hpp"
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>
#include <snitch/snitch_string.hpp>
#include <snitch/snitch_string_utility.hpp>

using namespace OpenVic;
using namespace std::string_view_literals;

TEST_CASE("ReliableUdpServer Constructors", "[ReliableUdpServer][ReliableUdpServer-constructor]") {
	std::unique_ptr<ReliableUdpServer> server = std::make_unique<ReliableUdpServer>();

	REQUIRE(server);
	CHECK_FALSE(server->is_listening());
}

static constexpr NetworkSocket::port_type PORT = 12345;
static constexpr IpAddress LOCALHOST = "127.0.0.1";
static constexpr uint32_t SLEEP_DURATION = 1000;
static constexpr uint64_t MAX_WAIT_MSEC = 100;

inline static memory::unique_ptr<ReliableUdpServer> create_server(IpAddress const& p_address, int p_port) {
	memory::unique_ptr<ReliableUdpServer> server = memory::make_unique<ReliableUdpServer>();

	NetworkError err = server->listen_to(PORT, LOCALHOST);
	REQUIRE(err == NetworkError::OK);
	REQUIRE(server->is_listening());
	CHECK_FALSE(server->is_connection_available());
	CHECK(server->get_max_pending_clients() == 16);

	return server;
}

inline static memory::unique_ptr<ReliableUdpClient> create_client(IpAddress const& p_address, int p_port) {
	memory::unique_ptr<ReliableUdpClient> client = memory::make_unique<ReliableUdpClient>();

	NetworkError err = client->connect_to(LOCALHOST, PORT);
	REQUIRE(err == NetworkError::OK);
	CHECK(client->is_bound());
	CHECK(client->is_connected());

	return client;
}

inline static void wait_for_condition(std::function<bool()> f_test) {
	const uint64_t time = GameManager::get_elapsed_milliseconds();
	while (!f_test() && (GameManager::get_elapsed_milliseconds() - time) < MAX_WAIT_MSEC) {
		std::this_thread::sleep_for(std::chrono::microseconds { SLEEP_DURATION });
	}
}

inline static memory::unique_ptr<ReliableUdpClient> accept_connection(ReliableUdpServer* p_server) {
	wait_for_condition([&]() {
		return p_server->poll() != NetworkError::OK || p_server->is_connection_available();
	});

	CHECK(p_server->poll() == NetworkError::OK);
	REQUIRE(p_server->is_connection_available());
	memory::unique_ptr<ReliableUdpClient> client_from_server = p_server->take_next_client_as<ReliableUdpClient>();
	REQUIRE(client_from_server);
	CHECK(client_from_server->is_bound());
	CHECK(client_from_server->is_connected());

	return client_from_server;
}

TEST_CASE("ReliableUdpServer accept connections and handle data", "[ReliableUdpServer][ReliableUdpServer-accept-connections]") {
	memory::unique_ptr<ReliableUdpServer> server = create_server(LOCALHOST, PORT);
	memory::unique_ptr<ReliableUdpClient> client = create_client(LOCALHOST, PORT);

	// Sending data from client to server.
	static constexpr std::string_view hello_world = "Hello World!"sv;

	memory::vector<uint8_t> data;
	zpp::bits::out out(data);

	CHECK(out(hello_world) == std::errc {});
	CHECK(client->set_packet(out.processed_data()) == NetworkError::OK);

	memory::unique_ptr<ReliableUdpClient> client_from_server = accept_connection(server.get());

	memory::string received;
	CHECK(client_from_server->in()(received) == std::errc {});
	CHECK(received == hello_world);

	out.reset();

	// Sending data from server to client.
	static constexpr float pi = 3.1415;

	CHECK(out(pi) == std::errc {});
	CHECK(client_from_server->set_packet(out.processed_data()) == NetworkError::OK);

	wait_for_condition([&]() {
		return client->available_packets() > 0;
	});

	CHECK_IF(client->get_acknowledged_sequences().size() == 1) {
		CHECK(client->get_acknowledged_sequences()[0] == 0);
	}

	CHECK(client->available_packets() > 0);

	float received_f;
	CHECK(client->in()(received_f) == std::errc {});
	CHECK(received_f == pi);

	out.reset();

	static constexpr float tau = pi * 2;

	CHECK(out(tau) == std::errc {});
	CHECK(client->set_packet(out.processed_data()) == NetworkError::OK);

	wait_for_condition([&]() {
		return server->poll() != NetworkError::OK;
	});

	CHECK_IF(client_from_server->get_acknowledged_sequences().size() == 1) {
		CHECK(client_from_server->get_acknowledged_sequences()[0] == 0);
	}

	CHECK(client_from_server->available_packets() > 0);

	CHECK(client_from_server->in()(received_f) == std::errc {});
	CHECK(received_f == tau);

	out.reset();
	client->clear_acknowledged_sequences();

	CHECK(out(pi) == std::errc {});
	CHECK(client_from_server->set_packet(out.processed_data()) == NetworkError::OK);

	wait_for_condition([&]() {
		return client->available_packets() > 0;
	});

	CHECK_IF(client->get_acknowledged_sequences().size() == 1) {
		CHECK(client->get_acknowledged_sequences()[0] == 1);
	}

	CHECK(client->available_packets() > 0);

	CHECK(client->in()(received_f) == std::errc {});
	CHECK(received_f == pi);

	client->close();
	server->close();
	CHECK_FALSE(server->is_listening());
}

TEST_CASE("ReliableUdpServer handle multiple clients", "[ReliableUdpServer][ReliableUdpServer-multiple-clients]") {
	memory::unique_ptr<ReliableUdpServer> server = create_server(LOCALHOST, PORT);

	memory::vector<memory::unique_ptr<ReliableUdpClient>> clients;

	memory::vector<uint8_t> data;
	zpp::bits::out out(data);

	for (int i = 0; i < 5; i++) {
		INFO(i);

		memory::unique_ptr<ReliableUdpClient> c = create_client(LOCALHOST, PORT);

		// Sending data from client to server.
		std::array<char, 1> hello_client {};
		std::to_chars(hello_client.data(), hello_client.data() + hello_client.size(), i);

		CHECK(out(std::string_view { hello_client.data(), hello_client.size() }) == std::errc {});
		CHECK(c->set_packet(out.processed_data()) == NetworkError::OK);
		out.reset();

		clients.push_back(std::move(c));
	}

	memory::vector<memory::string> packets;
	for (int i = 0; i < clients.size(); i++) {
		INFO(i);

		memory::unique_ptr<ReliableUdpClient> cfs = accept_connection(server.get());

		memory::string received_var;
		CHECK(cfs->in()(received_var) == std::errc {});
		packets.push_back(received_var);

		int received_int = -1;
		std::from_chars(received_var.data(), received_var.data() + received_var.size(), received_int);

		// Sending data from server to client.
		const float sent_float = 3.1415 + received_int;

		CHECK(out(sent_float) == std::errc {});
		CHECK(cfs->set_packet(out.processed_data()) == NetworkError::OK);
		out.reset();
	}

	CHECK(packets.size() == clients.size());

	std::sort(packets.begin(), packets.end());
	for (int i = 0; i < clients.size(); i++) {
		INFO(i);

		CHECK(packets[i] == memory::fmt::format("{}", i));
	}

	wait_for_condition([&]() {
		bool should_exit = true;
		for (memory::unique_ptr<ReliableUdpClient>& c : clients) {
			int count = c->available_packets();
			if (count < 0) {
				return true;
			}
			if (count == 0) {
				should_exit = false;
			}
		}
		return should_exit;
	});

	for (int i = 0; i < clients.size(); i++) {
		INFO(i);

		CHECK_IF(clients[i]->get_acknowledged_sequences().size() == 1) {
			CHECK(clients[i]->get_acknowledged_sequences()[0] == 0);
		}

		CHECK(clients[i]->available_packets() > 0);

		const float expected = 3.1415 + i;

		float received_f;
		CHECK(clients[i]->in()(received_f) == std::errc {});
		CHECK(received_f == expected);
	}

	for (memory::unique_ptr<ReliableUdpClient>& c : clients) {
		c->close();
	}
	server->close();
}

TEST_CASE("ReliableUdpServer refuse new connections after close", "[ReliableUdpServer][ReliableUdpServer-close-refuse]") {
	memory::unique_ptr<ReliableUdpServer> server = create_server(LOCALHOST, PORT);
	memory::unique_ptr<ReliableUdpClient> client = create_client(LOCALHOST, PORT);

	// Sending data from client to server.
	static constexpr std::string_view hello_world = "Hello World!"sv;

	memory::vector<uint8_t> data;
	zpp::bits::out out(data);

	CHECK(out(hello_world) == std::errc {});
	CHECK(client->set_packet(out.processed_data()) == NetworkError::OK);

	wait_for_condition([&]() {
		return server->poll() != NetworkError::OK || server->is_connection_available();
	});

	CHECK(client->get_acknowledged_sequences().empty());

	REQUIRE(server->is_connection_available());

	server->close();

	CHECK_FALSE(server->is_listening());
	CHECK_FALSE(server->is_connection_available());
}
