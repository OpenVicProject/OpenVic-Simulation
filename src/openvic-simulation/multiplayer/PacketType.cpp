#include "PacketType.hpp"

#include <memory>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/misc/GameAction.hpp"
#include "openvic-simulation/multiplayer/BaseMultiplayerManager.hpp"
#include "openvic-simulation/multiplayer/ClientManager.hpp"
#include "openvic-simulation/multiplayer/HostManager.hpp"
#include "openvic-simulation/multiplayer/HostSession.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;

bool PacketTypes::retransmit_packet_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	RetransmitData const* const* data_ptr = std::get_if<retransmit_packet.packet_id>(&argument);
	OV_ERR_FAIL_NULL_V(data_ptr, false);

	RetransmitData const& data = **data_ptr;
	OV_ERR_FAIL_COND_V(!PacketType::is_valid_type(data.packet_id), false);

	PacketType const& packet_type = PacketType::get_type_by_id(data.packet_id);
	OV_ERR_FAIL_COND_V(!packet_type.can_client_send, false);

	builder.put_back(data.client_id);
	builder.put_back(data.packet_id);
	return packet_type.create_packet(multiplayer_manager, data.argument, builder);
}

bool PacketTypes::retransmit_packet_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	uint64_t client_id = packet.read<uint64_t>();

	size_t start_index = packet.index();
	decltype(PacketType::packet_id) packet_id = packet.read<decltype(packet_id)>();

	OV_ERR_FAIL_COND_V(!PacketType::is_valid_type(packet_id), false);

	PacketType const& packet_type = PacketType::get_type_by_id(packet_id);
	OV_ERR_FAIL_COND_V(!packet_type.can_client_send, false);

	HostManager* host_manager = multiplayer_manager->cast_as<HostManager>();
	OV_ERR_FAIL_NULL_V(host_manager, false);

	return host_manager->send_data(client_id, packet.subspan(start_index));
}

bool PacketTypes::broadcast_packet_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	BroadcastData const* const* data_ptr = std::get_if<broadcast_packet.packet_id>(&argument);
	OV_ERR_FAIL_NULL_V(data_ptr, false);

	BroadcastData const& data = **data_ptr;
	OV_ERR_FAIL_COND_V(!PacketType::is_valid_type(data.packet_id), false);

	PacketType const& packet_type = PacketType::get_type_by_id(data.packet_id);
	OV_ERR_FAIL_COND_V(!packet_type.can_client_send, false);

	builder.put_back(data.packet_id);
	return packet_type.create_packet(multiplayer_manager, data.argument, builder);
}

bool PacketTypes::broadcast_packet_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	size_t start_index = packet.index();

	decltype(PacketType::packet_id) packet_id = packet.read<decltype(packet_id)>();
	OV_ERR_FAIL_COND_V(!PacketType::is_valid_type(packet_id), false);

	PacketType const& packet_type = PacketType::get_type_by_id(packet_id);
	OV_ERR_FAIL_COND_V(!packet_type.can_client_send, false);

	HostManager* host_manager = multiplayer_manager->cast_as<HostManager>();
	OV_ERR_FAIL_NULL_V(host_manager, false);

	return host_manager->broadcast_data(packet.subspan(start_index));
}

bool PacketTypes::send_raw_packet_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	std::span<uint8_t> const* data = std::get_if<send_raw_packet.packet_id>(&argument);
	OV_ERR_FAIL_NULL_V(data, false);

	builder.put_back<std::span<const uint8_t>>(*data);
	return true;
}

bool PacketTypes::send_raw_packet_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	std::span span = packet.read<std::span<uint8_t>>();
	multiplayer_manager->last_raw_packet.resize(span.size());
	std::uninitialized_copy_n(span.begin(), span.size(), multiplayer_manager->last_raw_packet.begin());
	return true;
}

bool PacketTypes::update_host_session_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	HostSession const* const* session_ptr = std::get_if<update_host_session.packet_id>(&argument);
	OV_ERR_FAIL_NULL_V(session_ptr, false);

	HostSession const& session = **session_ptr;
	builder.put_back(session);
	return true;
}

bool PacketTypes::update_host_session_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	ClientManager* client_manager = multiplayer_manager->cast_as<ClientManager>();
	OV_ERR_FAIL_NULL_V(client_manager, false);

	client_manager->host_session = packet.read<decltype(client_manager->host_session)>();
	return true;
}

bool PacketTypes::execute_game_action_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	game_action_t const* const* action_ptr = std::get_if<execute_game_action.packet_id>(&argument);
	OV_ERR_FAIL_NULL_V(action_ptr, false);

	builder.put_back(**action_ptr);
	return true;
}

bool PacketTypes::execute_game_action_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	game_action_t action = packet.read<decltype(action)>();

	multiplayer_manager->get_game_manager()->get_instance_manager()->queue_game_action(action.first, std::move(action).second);
	return true;
}

bool PacketTypes::send_chat_message_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	ChatMessageLog const* const* message_ptr = std::get_if<send_chat_message.packet_id>(&argument);
	OV_ERR_FAIL_NULL_V(message_ptr, false);

	ChatMessageLog const& message = **message_ptr;
	builder.put_back(message);
	return true;
}

bool PacketTypes::send_chat_message_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	GameManager* game_manager = multiplayer_manager->get_game_manager();
	ClientManager* client_manager = game_manager->get_client_manager();

	size_t index = packet.index();
	uint16_t from_id = packet.read<decltype(from_id)>();
	if (client_manager && client_manager->get_player()->get_client_id() == from_id) {
		return true;
	}

	packet.seek(index);
	game_manager->get_chat_manager()->_log_message(packet.read<ChatMessageLog>());
	return true;
}

bool PacketTypes::add_chat_group_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	std::span<uint64_t> const* clients_ptr = std::get_if<add_chat_group.packet_id>(&argument);
	OV_ERR_FAIL_NULL_V(clients_ptr, false);

	builder.put_back(*clients_ptr);
	return true;
}

bool PacketTypes::add_chat_group_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	multiplayer_manager->get_game_manager()->get_chat_manager()->_create_group(packet.read<memory::vector<uint64_t>>());
	return true;
}

bool PacketTypes::modify_chat_group_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	PacketChatGroupModifyData const* modify_data_ptr = std::get_if<modify_chat_group.packet_id>(&argument);
	OV_ERR_FAIL_NULL_V(modify_data_ptr, false);

	builder.put_back(*modify_data_ptr);
	return true;
}

bool PacketTypes::modify_chat_group_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	ChatGroup::index_type group_index = packet.read<decltype(group_index)>();

	multiplayer_manager->get_game_manager()->get_chat_manager()->_set_group(
		group_index, packet.read<memory::vector<BaseMultiplayerManager::client_id_type>>()
	);
	return true;
}

bool PacketTypes::delete_chat_group_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	ChatGroup::index_type const* group_id_ptr = std::get_if<delete_chat_group.packet_id>(&argument);
	OV_ERR_FAIL_NULL_V(group_id_ptr, false);

	builder.put_back(*group_id_ptr);
	return true;
}

bool PacketTypes::delete_chat_group_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	multiplayer_manager->get_game_manager()->get_chat_manager()->_delete_group(packet.read<ChatGroup::index_type>());
	return true;
}

bool PacketTypes::send_battleplan_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	// TODO: needs battleplan implementation
	return true;
}

bool PacketTypes::send_battleplan_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	// TODO: need battleplan implementation
	return true;
}

bool PacketTypes::notify_player_left_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	// TODO: needs player left notify implementation
	return true;
}

bool PacketTypes::notify_player_left_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	// TODO: needs player left notify implementation
	return true;
}

bool PacketTypes::set_ready_status_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder
) {
	// TODO: needs player's set ready status implementation
	return true;
}

bool PacketTypes::set_ready_status_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	// TODO: needs player's set ready status implementation
	return true;
}
