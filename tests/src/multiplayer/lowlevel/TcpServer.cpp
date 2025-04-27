#include "openvic-simulation/multiplayer/lowlevel/TcpServer.hpp"

#include <chrono>
#include <memory>
#include <string_view>
#include <thread>

#include <range/v3/algorithm/fill.hpp>

#include <zpp_bits.h>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/TcpPacketStream.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>
#include <snitch/snitch_string.hpp>
#include <snitch/snitch_string_utility.hpp>

using namespace OpenVic;
using namespace std::string_view_literals;

TEST_CASE("TcpServer Constructors", "[TcpServer][TcpServer-constructor]") {
	memory::unique_ptr<TcpServer> server = memory::make_unique<TcpServer>();

	REQUIRE(server);
	CHECK_FALSE(server->is_listening());
}

static constexpr NetworkSocket::port_type PORT = 12345;
static constexpr IpAddress LOCALHOST = "127.0.0.1";
static constexpr uint32_t SLEEP_DURATION = 1000;
static constexpr uint64_t MAX_WAIT_MSEC = 2000;

inline static memory::unique_ptr<TcpServer> create_server(IpAddress const& p_address, int p_port) {
	memory::unique_ptr<TcpServer> server = memory::make_unique<TcpServer>();

	NetworkError err = server->listen_to(PORT, LOCALHOST);
	REQUIRE(err == NetworkError::OK);
	REQUIRE(server->is_listening());
	CHECK_FALSE(server->is_connection_available());

	return server;
}

inline static memory::unique_ptr<TcpPacketStream> create_client(IpAddress const& p_address, int p_port) {
	memory::unique_ptr<TcpPacketStream> client = memory::make_unique<TcpPacketStream>();

	NetworkError err = client->connect_to(LOCALHOST, PORT);
	REQUIRE(err == NetworkError::OK);
	CHECK(client->get_connected_address() == LOCALHOST);
	CHECK(client->get_connected_port() == PORT);
	CHECK(client->get_status() == TcpPacketStream::Status::CONNECTING);

	return client;
}

inline static void wait_for_condition(std::function<bool()> f_test) {
	const uint64_t time = GameManager::get_elapsed_milliseconds();
	while (!f_test() && (GameManager::get_elapsed_milliseconds() - time) < MAX_WAIT_MSEC) {
		std::this_thread::sleep_for(std::chrono::microseconds { SLEEP_DURATION });
	}
}

inline static memory::unique_ptr<TcpPacketStream> accept_connection(TcpServer* p_server) {
	wait_for_condition([&]() {
		return p_server->is_connection_available();
	});

	REQUIRE(p_server->is_connection_available());
	memory::unique_ptr<TcpPacketStream> client_from_server = p_server->take_packet_stream();
	REQUIRE(client_from_server);
	CHECK(client_from_server->get_connected_address() == LOCALHOST);
	CHECK(client_from_server->get_status() == TcpPacketStream::Status::CONNECTED);

	return client_from_server;
}

TEST_CASE("TcpServer accept connections and handle data", "[TcpServer][TcpServer-accept-connections]") {
	memory::unique_ptr<TcpServer> server = create_server(LOCALHOST, PORT);
	memory::unique_ptr<TcpPacketStream> client = create_client(LOCALHOST, PORT);
	memory::unique_ptr<TcpPacketStream> client_from_server = accept_connection(server.get());

	wait_for_condition([&]() {
		return client->poll() != NetworkError::OK || client->get_status() == TcpPacketStream::Status::CONNECTED;
	});

	CHECK(client->get_status() == TcpPacketStream::Status::CONNECTED);

	// Sending data from client to server.
	static constexpr std::string_view hello_world = "Hello World!"sv;

	memory::vector<uint8_t> data;
	zpp::bits::out out(data);

	CHECK(out(hello_world) == std::errc {});
	CHECK(client->set_data(out.processed_data()) == NetworkError::OK);

	zpp::bits::in in(data);

	ranges::fill(in.data(), 0);
	CHECK(client_from_server->get_data(in.data()) == NetworkError::OK);

	memory::string received;
	CHECK(in(received) == std::errc {});
	CHECK(received == hello_world);

	out.reset();
	in.reset();

	// Sending data from server to client.
	static constexpr float pi = 3.1415;
	CHECK(out(pi) == std::errc {});
	CHECK(client_from_server->set_data(out.processed_data()) == NetworkError::OK);

	ranges::fill(in.data(), 0);
	CHECK(client->get_data(in.data()) == NetworkError::OK);

	float received_f;
	CHECK(in(received_f) == std::errc {});
	CHECK(received_f == pi);

	client->close();
	server->close();
	CHECK_FALSE(server->is_listening());
}

TEST_CASE("TcpServer handle multiple clients", "[TcpServer][TcpServer-multiple-clients]") {
	memory::unique_ptr<TcpServer> server = create_server(LOCALHOST, PORT);

	memory::vector<memory::unique_ptr<TcpPacketStream>> clients;
	for (int i = 0; i < 5; i++) {
		clients.push_back(create_client(LOCALHOST, PORT));
	}

	memory::vector<memory::unique_ptr<TcpPacketStream>> clients_from_server;
	for (int i = 0; i < 5; i++) {
		INFO(i);

		clients_from_server.push_back(accept_connection(server.get()));
	}

	wait_for_condition([&]() {
		bool should_exit = true;
		for (memory::unique_ptr<TcpPacketStream>& c : clients) {
			if (c->poll() != NetworkError::OK) {
				return true;
			}
			TcpPacketStream::Status status = c->get_status();
			if (status != TcpPacketStream::Status::CONNECTED && status != TcpPacketStream::Status::CONNECTING) {
				return true;
			}
			if (status != TcpPacketStream::Status::CONNECTED) {
				should_exit = false;
			}
		}
		return should_exit;
	});

	for (int i = 0; i < clients.size(); i++) {
		INFO(i);

		REQUIRE(clients[i]->get_status() == TcpPacketStream::Status::CONNECTED);
	}

	// Sending data from each client to server.
	memory::vector<uint8_t> data;
	zpp::bits::out out(data);
	zpp::bits::in in(data);
	for (int i = 0; i < clients.size(); i++) {
		INFO(i);

		memory::string hello_client = memory::fmt::format("Hello {}", i);
		CHECK(out(hello_client) == std::errc {});
		CHECK(clients[i]->set_data(out.processed_data()) == NetworkError::OK);

		ranges::fill(in.data(), 0);
		CHECK(clients_from_server[i]->get_data(in.data()) == NetworkError::OK);

		memory::string received;
		CHECK(in(received) == std::errc {});
		CHECK(received == hello_client);
		out.reset();
		in.reset();
	}

	for (memory::unique_ptr<TcpPacketStream>& c : clients) {
		c->close();
	}
	server->close();
}

TEST_CASE("TcpServer refuse new connections after close", "[TcpServer][TcpServer-close-refuse]") {
	memory::unique_ptr<TcpServer> server = create_server(LOCALHOST, PORT);
	memory::unique_ptr<TcpPacketStream> client = create_client(LOCALHOST, PORT);
	memory::unique_ptr<TcpPacketStream> client_from_server = accept_connection(server.get());

	wait_for_condition([&]() {
		return client->poll() != NetworkError::OK || client->get_status() == TcpPacketStream::Status::CONNECTED;
	});

	CHECK(client->get_status() == TcpPacketStream::Status::CONNECTED);

	// Sending data from client to server.
	static constexpr std::string_view hello_world = "Hello World!"sv;

	memory::vector<uint8_t> data;
	zpp::bits::out out(data);

	CHECK(out(hello_world) == std::errc {});
	CHECK(client->set_data(out.processed_data()) == NetworkError::OK);

	zpp::bits::in in(data);

	ranges::fill(in.data(), 0);
	CHECK(client_from_server->get_data(in.data()) == NetworkError::OK);

	memory::string received;
	CHECK(in(received) == std::errc {});
	CHECK(received == hello_world);

	client->close();
	server->close();
	CHECK_FALSE(server->is_listening());

	// Make sure the client times out in less than the wait time.
	memory::unique_ptr<TcpPacketStream> new_client = memory::make_unique<TcpPacketStream>();
	int timeout = new_client->get_timeout_seconds();
	new_client->set_timeout_seconds(1);
	REQUIRE(new_client->connect_to(LOCALHOST, PORT) == NetworkError::OK);
	CHECK(new_client->get_connected_address() == LOCALHOST);
	CHECK(new_client->get_connected_port() == PORT);
	CHECK(new_client->get_status() == TcpPacketStream::Status::CONNECTING);

	CHECK_FALSE(server->is_connection_available());

	wait_for_condition([&]() {
		return new_client->poll() != NetworkError::OK || new_client->get_status() == TcpPacketStream::Status::ERROR;
	});

	CHECK_FALSE(server->is_connection_available());

	CHECK(new_client->get_status() == TcpPacketStream::Status::ERROR);
	new_client->close();
	CHECK(new_client->get_status() == TcpPacketStream::Status::NONE);
}

TEST_CASE("TcpServer disconnect client", "[TcpServer][TcpServer-disconnect-client]") {
	memory::unique_ptr<TcpServer> server = create_server(LOCALHOST, PORT);
	memory::unique_ptr<TcpPacketStream> client = create_client(LOCALHOST, PORT);
	memory::unique_ptr<TcpPacketStream> client_from_server = accept_connection(server.get());

	wait_for_condition([&]() {
		return client->poll() != NetworkError::OK || client->get_status() == TcpPacketStream::Status::CONNECTED;
	});

	CHECK(client->get_status() == TcpPacketStream::Status::CONNECTED);

	// Sending data from client to server.
	static constexpr std::string_view hello_world = "Hello World!"sv;

	memory::vector<uint8_t> data;
	zpp::bits::out out(data);

	CHECK(out(hello_world) == std::errc {});
	CHECK(client->set_data(out.processed_data()) == NetworkError::OK);

	zpp::bits::in in(data);

	ranges::fill(in.data(), 0);
	CHECK(client_from_server->get_data(in.data()) == NetworkError::OK);

	memory::string received;
	CHECK(in(received) == std::errc {});
	CHECK(received == hello_world);

	out.reset();
	in.reset();

	client_from_server->close();
	server->close();
	CHECK_FALSE(server->is_listening());

	// Wait for disconnection
	wait_for_condition([&]() {
		return client->poll() != NetworkError::OK || client->get_status() == TcpPacketStream::Status::NONE;
	});

	// Wait for disconnection
	wait_for_condition([&]() {
		return client_from_server->poll() != NetworkError::OK ||
			client_from_server->get_status() == TcpPacketStream::Status::NONE;
	});

	CHECK(client->get_status() == TcpPacketStream::Status::NONE);
	CHECK(client_from_server->get_status() == TcpPacketStream::Status::NONE);

	in.reset();
	ranges::fill(in.data(), 0);
	CHECK(client_from_server->get_data(in.data()) == NetworkError::FAILED);

	CHECK(in(received) == std::errc {});
	CHECK(received.empty());

	in.reset();
	ranges::fill(in.data(), 0);
	CHECK(client->get_data(in.data()) == NetworkError::FAILED);

	CHECK(in(received) == std::errc {});
	CHECK(received.empty());
}
