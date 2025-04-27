#include "openvic-simulation/multiplayer/lowlevel/UdpServer.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"
#include "openvic-simulation/multiplayer/lowlevel/UdpClient.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>
#include <snitch/snitch_string.hpp>
#include <snitch/snitch_string_utility.hpp>

using namespace OpenVic;
using namespace std::string_view_literals;

TEST_CASE("UdpServer Constructors", "[UdpServer][UdpServer-constructor]") {
	memory::unique_ptr<UdpServer> server = memory::make_unique<UdpServer>();

	REQUIRE(server);
	CHECK_FALSE(server->is_listening());
}

static constexpr NetworkSocket::port_type PORT = 12345;
static constexpr IpAddress LOCALHOST = "127.0.0.1";
static constexpr uint32_t SLEEP_DURATION = 1000;
static constexpr uint64_t MAX_WAIT_MSEC = 100;

inline static memory::unique_ptr<UdpServer> create_server(IpAddress const& p_address, int p_port) {
	memory::unique_ptr<UdpServer> server = memory::make_unique<UdpServer>();

	NetworkError err = server->listen_to(PORT, LOCALHOST);
	REQUIRE(err == NetworkError::OK);
	REQUIRE(server->is_listening());
	CHECK_FALSE(server->is_connection_available());
	CHECK(server->get_max_pending_clients() == 16);

	return server;
}

inline static memory::unique_ptr<UdpClient> create_client(IpAddress const& p_address, int p_port) {
	memory::unique_ptr<UdpClient> client = memory::make_unique<UdpClient>();

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

inline static memory::unique_ptr<UdpClient> accept_connection(UdpServer* p_server) {
	wait_for_condition([&]() {
		return p_server->poll() != NetworkError::OK || p_server->is_connection_available();
	});

	CHECK(p_server->poll() == NetworkError::OK);
	REQUIRE(p_server->is_connection_available());
	memory::unique_ptr<UdpClient> client_from_server = p_server->take_next_client_as<UdpClient>();
	REQUIRE(client_from_server);
	CHECK(client_from_server->is_bound());
	CHECK(client_from_server->is_connected());

	return client_from_server;
}

TEST_CASE("UdpServer accept connections and handle data", "[UdpServer][UdpServer-accept-connections]") {
	memory::unique_ptr<UdpServer> server = create_server(LOCALHOST, PORT);
	memory::unique_ptr<UdpClient> client = create_client(LOCALHOST, PORT);

	// Sending data from client to server.
	static constexpr std::string_view hello_world = "Hello World!"sv;

	PacketBuilder pb;
	pb.put_back(hello_world);
	CHECK(client->set_packet(pb) == NetworkError::OK);

	memory::unique_ptr<UdpClient> client_from_server = accept_connection(server.get());
	CHECK(client_from_server->packet_span().read<std::string_view>() == hello_world);

	pb.clear();

	// Sending data from server to client.
	static constexpr float pi = 3.1415;

	pb.put_back(pi);
	CHECK(client_from_server->set_packet(pb) == NetworkError::OK);

	wait_for_condition([&]() {
		return client->available_packets() > 0;
	});

	CHECK(client->available_packets() > 0);

	CHECK(client->packet_span().read<float>() == pi);

	client->close();
	server->close();
	CHECK_FALSE(server->is_listening());
}

TEST_CASE("UdpServer handle multiple clients", "[UdpServer][UdpServer-multiple-clients]") {
	memory::unique_ptr<UdpServer> server = create_server(LOCALHOST, PORT);

	memory::vector<memory::unique_ptr<UdpClient>> clients;
	PacketBuilder pb;
	for (int i = 0; i < 5; i++) {
		memory::unique_ptr<UdpClient> c = create_client(LOCALHOST, PORT);

		// Sending data from client to server.
		std::array<char, 1> hello_client {};
		std::to_chars(hello_client.data(), hello_client.data() + hello_client.size(), i);

		pb.put_back(std::string_view { hello_client.data(), hello_client.size() });
		CHECK(c->set_packet(pb) == NetworkError::OK);
		pb.clear();

		clients.push_back(std::move(c));
	}

	memory::vector<std::string> packets;
	for (int i = 0; i < clients.size(); i++) {
		memory::unique_ptr<UdpClient> cfs = accept_connection(server.get());

		std::string received_var = cfs->packet_span().read<std::string>();
		packets.push_back(received_var);

		int received_int = -1;
		std::from_chars(received_var.data(), received_var.data() + received_var.size(), received_int);

		// Sending data from server to client.
		const float sent_float = 3.1415 + received_int;

		pb.put_back(sent_float);
		CHECK(cfs->set_packet(pb) == NetworkError::OK);
		pb.clear();
	}

	CHECK(packets.size() == clients.size());

	std::sort(packets.begin(), packets.end());
	for (int i = 0; i < clients.size(); i++) {
		CHECK(packets[i] == std::to_string(i));
	}

	wait_for_condition([&]() {
		bool should_exit = true;
		for (memory::unique_ptr<UdpClient>& c : clients) {
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
		CHECK(clients[i]->available_packets() > 0);

		const float expected = 3.1415 + i;

		CHECK(clients[i]->packet_span().read<float>() == expected);
	}

	for (memory::unique_ptr<UdpClient>& c : clients) {
		c->close();
	}
	server->close();
}

TEST_CASE("UdpServer refuse new connections after close", "[UdpServer][UdpServer-close-refuse]") {
	memory::unique_ptr<UdpServer> server = create_server(LOCALHOST, PORT);
	memory::unique_ptr<UdpClient> client = create_client(LOCALHOST, PORT);

	// Sending data from client to server.
	static constexpr std::string_view hello_world = "Hello World!"sv;

	PacketBuilder pb;
	pb.put_back(hello_world);
	CHECK(client->set_packet(pb) == NetworkError::OK);

	wait_for_condition([&]() {
		return server->poll() != NetworkError::OK || server->is_connection_available();
	});

	REQUIRE(server->is_connection_available());

	server->close();

	CHECK_FALSE(server->is_listening());
	CHECK_FALSE(server->is_connection_available());
}
