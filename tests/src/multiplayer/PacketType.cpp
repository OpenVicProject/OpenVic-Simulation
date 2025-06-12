#include "openvic-simulation/multiplayer/PacketType.hpp"

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/multiplayer/ClientManager.hpp"
#include "openvic-simulation/multiplayer/HostManager.hpp"
#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>
#include <snitch/snitch_string.hpp>
#include <snitch/snitch_string_utility.hpp>

namespace snitch {
	extern OpenVic::GameManager* game_manager;
}

using namespace OpenVic;
using namespace std::string_view_literals;

static constexpr NetworkSocket::port_type PORT = 12345;
static constexpr IpAddress LOCALHOST = "127.0.0.1";
static constexpr uint32_t SLEEP_DURATION = 1000;
static constexpr uint64_t MAX_WAIT_MSEC = 100;

inline static HostManager* create_host(IpAddress const& p_address, int p_port) {
	snitch::game_manager->create_host();
	HostManager* host_manager = snitch::game_manager->get_host_manager();

	REQUIRE(host_manager->listen(p_port, p_address));
	REQUIRE(host_manager->get_server().is_listening());
	CHECK_FALSE(host_manager->get_server().is_connection_available());
	CHECK(host_manager->get_server().get_max_pending_clients() == 16);

	return host_manager;
}

inline static ClientManager* create_client(IpAddress const& p_address, int p_port) {
	snitch::game_manager->create_client();
	ClientManager* client = snitch::game_manager->get_client_manager();

	REQUIRE(client->connect_to(p_address, p_port));
	CHECK(client->get_client().is_bound());
	CHECK(client->get_client().is_connected());

	return client;
}

inline static void wait_for_condition(std::function<bool()> f_test) {
	const uint64_t time = GameManager::get_elapsed_milliseconds();
	while (!f_test() && (GameManager::get_elapsed_milliseconds() - time) < MAX_WAIT_MSEC) {
		std::this_thread::sleep_for(std::chrono::microseconds { SLEEP_DURATION });
	}
}

TEST_CASE("PacketType send_raw_packet", "[PacketType][PacketType-send_raw_packet]") {
	HostManager* host_manager = create_host(LOCALHOST, PORT);
	ClientManager* client_manager = create_client(LOCALHOST, PORT);
	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});

	static constexpr int32_t _256 = 256;

	PacketBuilder builder;
	builder.put_back(_256);
	CHECK(host_manager->broadcast_packet(PacketTypes::send_raw_packet, builder));
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});

	CHECK(client_manager->get_last_raw_packet().read<decltype(_256)>() == _256);

	client_manager->close();
	host_manager->close();
}

TEST_CASE("PacketType update_host_session", "[PacketType][PacketType-update_host_session]") {
	HostManager* host_manager = create_host(LOCALHOST, PORT);
	ClientManager* client_manager = create_client(LOCALHOST, PORT);
	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});

	host_manager->get_host_session().set_game_name("NewHostSessionName");
	CHECK(host_manager->broadcast_packet(PacketTypes::update_host_session, &host_manager->get_host_session()));
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});

	CHECK(client_manager->get_host_session().get_game_name() == "NewHostSessionName"sv);

	client_manager->close();
	host_manager->close();
}
