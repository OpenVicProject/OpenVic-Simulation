#include "PacketType.hpp"

#include <any>
#include <memory>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/misc/GameAction.hpp"
#include "openvic-simulation/multiplayer/BaseMultiplayerManager.hpp"
#include "openvic-simulation/multiplayer/ClientManager.hpp"
#include "openvic-simulation/multiplayer/HostManager.hpp"
#include "openvic-simulation/multiplayer/HostSession.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;

bool PacketTypes::retransmit_packet_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, std::any const& argument, PacketBuilder<>& builder
) {
	RetransmitData const* data = std::any_cast<RetransmitData>(&argument);
	OV_ERR_FAIL_COND_V(data == nullptr, false);
	OV_ERR_FAIL_COND_V(!PacketType::is_valid_type(data->packet_id), false);

	PacketType const& packet_type = PacketType::get_type_by_id(data->packet_id);
	OV_ERR_FAIL_COND_V(!packet_type.can_client_send, false);

	builder.put_back(data->client_id);
	builder.put_back(data->packet_id);
	return packet_type.create_packet(multiplayer_manager, data->argument, builder);
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

	return host_manager->send_data(client_id, { packet.data() + start_index, packet.size() - start_index });
}

bool PacketTypes::broadcast_packet_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, std::any const& argument, PacketBuilder<>& builder
) {
	BroadcastData const* data = std::any_cast<BroadcastData>(&argument);
	OV_ERR_FAIL_NULL_V(data, false);
	OV_ERR_FAIL_COND_V(!PacketType::is_valid_type(data->packet_id), false);

	PacketType const& packet_type = PacketType::get_type_by_id(data->packet_id);
	OV_ERR_FAIL_COND_V(!packet_type.can_client_send, false);

	builder.put_back(data->packet_id);
	return packet_type.create_packet(multiplayer_manager, data->argument, builder);
}

bool PacketTypes::broadcast_packet_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	decltype(PacketType::packet_id) packet_id = packet.read<decltype(packet_id)>();
	OV_ERR_FAIL_COND_V(!PacketType::is_valid_type(packet_id), false);

	PacketType const& packet_type = PacketType::get_type_by_id(packet_id);
	OV_ERR_FAIL_COND_V(!packet_type.can_client_send, false);

	HostManager* host_manager = multiplayer_manager->cast_as<HostManager>();
	OV_ERR_FAIL_NULL_V(host_manager, false);

	return host_manager->broadcast_data(packet);
}

bool PacketTypes::send_raw_packet_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, std::any const& argument, PacketBuilder<>& builder
) {
	std::vector<uint8_t> const* data = std::any_cast<std::vector<uint8_t>>(&argument);
	if (data == nullptr) {
		data = std::any_cast<PacketBuilder<>>(&argument);
	}
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
	BaseMultiplayerManager const* multiplayer_manager, std::any const& argument, PacketBuilder<>& builder
) {
	HostSession const* const* session_ptr = std::any_cast<HostSession const*>(&argument);
	if (!session_ptr) {
		session_ptr = std::any_cast<HostSession*>(&argument);
	}
	OV_ERR_FAIL_NULL_V(session_ptr, false);

	HostSession const& session = **session_ptr;
	std::vector<uint8_t> bytes = session.serialize();
	builder.insert(builder.end(), bytes.begin(), bytes.end());
	return true;
}

bool PacketTypes::update_host_session_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	ClientManager* client_manager = multiplayer_manager->cast_as<ClientManager>();
	OV_ERR_FAIL_NULL_V(client_manager, false);

	client_manager->host_session = HostSession::deseralize(packet);
	return true;
}

bool PacketTypes::execute_game_action_send_callback(
	BaseMultiplayerManager const* multiplayer_manager, std::any const& argument, PacketBuilder<>& builder
) {
	game_action_t const* action_ptr = std::any_cast<game_action_t>(&argument);
	OV_ERR_FAIL_NULL_V(action_ptr, false);

	builder.put_back(*action_ptr);
	return true;
}

bool PacketTypes::execute_game_action_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet) {
	game_action_t action = packet.read<decltype(action)>();

	multiplayer_manager->get_game_manager()->get_instance_manager()->queue_game_action(action.first, std::move(action).second);
	return true;
}
