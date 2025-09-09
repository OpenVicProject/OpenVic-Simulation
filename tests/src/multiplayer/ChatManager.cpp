#include "openvic-simulation/multiplayer/ChatManager.hpp"

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/multiplayer/BaseMultiplayerManager.hpp"
#include "openvic-simulation/multiplayer/ClientManager.hpp"
#include "openvic-simulation/multiplayer/HostManager.hpp"
#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>
#include <snitch/snitch_string.hpp>
#include <snitch/snitch_string_utility.hpp>

namespace snitch {
	extern OpenVic::GameManager* game_manager;
	extern OpenVic::GameManager* get_mp_game_managers(size_t index);
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

inline static ClientManager* create_client( //
	BaseMultiplayerManager::client_id_type client_id, IpAddress const& p_address, int p_port
) {
	ClientManager* client;
	if (client_id == ClientManager::HOST_ID) {
		snitch::game_manager->create_client();
		client = snitch::game_manager->get_client_manager();
	} else {
		GameManager* manager = snitch::get_mp_game_managers(client_id);
		manager->create_client();
		client = manager->get_client_manager();
	}

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

TEST_CASE("ChatManager send_private_message", "[ChatManager][ChatManager-send_private_message]") {
	HostManager* host_manager = create_host(LOCALHOST, PORT);
	ClientManager* client_manager = create_client(ClientManager::HOST_ID, LOCALHOST, PORT);
	ChatManager& chat_manager = *snitch::game_manager->get_chat_manager();

	ClientManager* client_manager_0 = create_client(0, LOCALHOST, PORT);
	ChatManager& chat_manager_0 = *snitch::get_mp_game_managers(0)->get_chat_manager();

	ClientManager* client_manager_1 = create_client(1, LOCALHOST, PORT);
	ChatManager& chat_manager_1 = *snitch::get_mp_game_managers(1)->get_chat_manager();

	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_0->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_1->poll() > 0;
	});

	static constexpr std::string_view hello_world = "Hello World";

	CHECK(chat_manager.send_private_message(1, hello_world));
	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_0->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_1->poll() > 0;
	});

	ChatMessageLog const& log = chat_manager_0.get_message_logs().front();
	CHECK(log.from_id == 0);
	CHECK(log.data.type == ChatManager::MessageType::PRIVATE);
	CHECK(log.data.message == hello_world);
	CHECK(log.data.from_index == 1);

	client_manager->close();
	client_manager_0->close();
	client_manager_1->close();
	host_manager->close();
}

TEST_CASE("ChatManager send_group_message", "[ChatManager][ChatManager-send_group_message]") {
	HostManager* host_manager = create_host(LOCALHOST, PORT);
	ClientManager* client_manager = create_client(ClientManager::HOST_ID, LOCALHOST, PORT);
	ChatManager& chat_manager = *snitch::game_manager->get_chat_manager();

	ClientManager* client_manager_0 = create_client(0, LOCALHOST, PORT);
	ChatManager& chat_manager_0 = *snitch::get_mp_game_managers(0)->get_chat_manager();

	ClientManager* client_manager_1 = create_client(1, LOCALHOST, PORT);
	ChatManager& chat_manager_1 = *snitch::get_mp_game_managers(1)->get_chat_manager();

	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_0->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_1->poll() > 0;
	});

	chat_manager.create_group({ client_manager_0->get_client_id(), client_manager_1->get_client_id() });

	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_0->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_1->poll() > 0;
	});

	ChatGroup const& local_group = chat_manager.get_group(0);
	CHECK(local_group.get_index() == 0);
	CHECK(local_group.get_clients().size() == 2);
	CHECK(local_group.get_clients()[0] == client_manager_0->get_client_id());
	CHECK(local_group.get_clients()[1] == client_manager_1->get_client_id());

	ChatGroup const& group_0 = chat_manager_0.get_group(0);
	CHECK(group_0.get_index() == 0);
	CHECK(group_0.get_clients().size() == 2);
	CHECK(group_0.get_clients()[0] == client_manager_0->get_client_id());
	CHECK(group_0.get_clients()[1] == client_manager_1->get_client_id());

	ChatGroup const& group_1 = chat_manager_1.get_group(0);
	CHECK(group_1.get_index() == 0);
	CHECK(group_1.get_clients().size() == 2);
	CHECK(group_1.get_clients()[0] == client_manager_0->get_client_id());
	CHECK(group_1.get_clients()[1] == client_manager_1->get_client_id());

	static constexpr std::string_view hello_world = "Hello World";

	CHECK(chat_manager.send_group_message(local_group, hello_world));

	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_0->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_1->poll() > 0;
	});

	ChatMessageLog const& log = chat_manager_0.get_message_logs().front();
	CHECK(log.from_id == 0);
	CHECK(log.data.type == ChatManager::MessageType::GROUP);
	CHECK(log.data.message == hello_world);
	CHECK(log.data.from_index == 0);

	ChatMessageLog const& log2 = chat_manager_1.get_message_logs().front();
	CHECK(log2.from_id == 0);
	CHECK(log2.data.type == ChatManager::MessageType::GROUP);
	CHECK(log2.data.message == hello_world);
	CHECK(log2.data.from_index == 0);

	client_manager->close();
	client_manager_0->close();
	client_manager_1->close();
	host_manager->close();
}

TEST_CASE("ChatManager send_public_message", "[ChatManager][ChatManager-send_public_message]") {
	HostManager* host_manager = create_host(LOCALHOST, PORT);
	ClientManager* client_manager = create_client(ClientManager::HOST_ID, LOCALHOST, PORT);
	ChatManager& chat_manager = *snitch::game_manager->get_chat_manager();

	ClientManager* client_manager_0 = create_client(0, LOCALHOST, PORT);
	ChatManager& chat_manager_0 = *snitch::get_mp_game_managers(0)->get_chat_manager();

	ClientManager* client_manager_1 = create_client(1, LOCALHOST, PORT);
	ChatManager& chat_manager_1 = *snitch::get_mp_game_managers(1)->get_chat_manager();
	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_0->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_1->poll() > 0;
	});

	static constexpr std::string_view hello_world = "Hello World";

	CHECK(chat_manager.send_public_message(hello_world));
	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_0->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_1->poll() > 0;
	});

	ChatMessageLog const& log = chat_manager_0.get_message_logs().front();
	CHECK(log.from_id == 0);
	CHECK(log.data.type == ChatManager::MessageType::PUBLIC);
	CHECK(log.data.message == hello_world);

	ChatMessageLog const& log2 = chat_manager_1.get_message_logs().front();
	CHECK(log2.from_id == 0);
	CHECK(log2.data.type == ChatManager::MessageType::PUBLIC);
	CHECK(log2.data.message == hello_world);

	client_manager->close();
	client_manager_0->close();
	client_manager_1->close();
	host_manager->close();
}

TEST_CASE("ChatManager set_group", "[ChatManager][ChatManager-set_group]") {
	HostManager* host_manager = create_host(LOCALHOST, PORT);
	ClientManager* client_manager = create_client(ClientManager::HOST_ID, LOCALHOST, PORT);
	ChatManager& chat_manager = *snitch::game_manager->get_chat_manager();

	ClientManager* client_manager_0 = create_client(0, LOCALHOST, PORT);
	ChatManager& chat_manager_0 = *snitch::get_mp_game_managers(0)->get_chat_manager();

	ClientManager* client_manager_1 = create_client(1, LOCALHOST, PORT);
	ChatManager& chat_manager_1 = *snitch::get_mp_game_managers(1)->get_chat_manager();

	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_0->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_1->poll() > 0;
	});

	chat_manager.create_group({ client_manager_0->get_client_id(), client_manager_1->get_client_id() });

	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_0->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_1->poll() > 0;
	});

	ChatGroup const& local_group = chat_manager.get_group(0);
	CHECK(local_group.get_index() == 0);
	CHECK(local_group.get_clients().size() == 2);
	CHECK(local_group.get_clients()[0] == client_manager_0->get_client_id());
	CHECK(local_group.get_clients()[1] == client_manager_1->get_client_id());

	ChatGroup const& group_0 = chat_manager_0.get_group(0);
	CHECK(group_0.get_index() == 0);
	CHECK(group_0.get_clients().size() == 2);
	CHECK(group_0.get_clients()[0] == client_manager_0->get_client_id());
	CHECK(group_0.get_clients()[1] == client_manager_1->get_client_id());

	ChatGroup const& group_1 = chat_manager_1.get_group(0);
	CHECK(group_1.get_index() == 0);
	CHECK(group_1.get_clients().size() == 2);
	CHECK(group_1.get_clients()[0] == client_manager_0->get_client_id());
	CHECK(group_1.get_clients()[1] == client_manager_1->get_client_id());

	chat_manager.set_group(local_group, { client_manager->get_client_id(), client_manager_1->get_client_id() });

	wait_for_condition([&]() {
		return host_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_0->poll() > 0;
	});
	wait_for_condition([&]() {
		return client_manager_1->poll() > 0;
	});

	CHECK(local_group.get_index() == 0);
	CHECK(local_group.get_clients().size() == 2);
	CHECK(local_group.get_clients()[0] == client_manager->get_client_id());
	CHECK(local_group.get_clients()[1] == client_manager_1->get_client_id());

	CHECK(group_0.get_index() == 0);
	CHECK(group_0.get_clients().size() == 2);
	CHECK(group_0.get_clients()[0] == client_manager->get_client_id());
	CHECK(group_0.get_clients()[1] == client_manager_1->get_client_id());

	CHECK(group_1.get_index() == 0);
	CHECK(group_1.get_clients().size() == 2);
	CHECK(group_1.get_clients()[0] == client_manager->get_client_id());
	CHECK(group_1.get_clients()[1] == client_manager_1->get_client_id());

	client_manager->close();
	client_manager_0->close();
	client_manager_1->close();
	host_manager->close();
}
