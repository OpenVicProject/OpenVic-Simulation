
#include "ClientManager.hpp"

#include "openvic-simulation/multiplayer/PacketType.hpp"
#include "openvic-simulation/multiplayer/lowlevel/HostnameAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"
#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpClient.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;

bool ClientManager::connect_to(HostnameAddress const& address, NetworkSocket::port_type port) {
	NetworkError err = client.connect_to(address.resolved_address(), port);
	OV_ERR_FAIL_COND_V(err != NetworkError::OK, false);
	PacketBuilder builder;
	builder.put_back<uint8_t>(1);
	return client.set_packet(builder) == NetworkError::OK;
}

template<PacketType const& type>
bool ClientManager::_send_packet_to_host(auto const& argument) {
	PacketBuilder builder;
	builder.put_back(type.packet_id);
	if (!type.create_packet(this, argument, builder)) {
		return false;
	}

	if (!add_packet_to_cache(builder)) {
		return false;
	}

	client.set_packet(builder);
	return true;
}

bool ClientManager::broadcast_packet(PacketType const& type, std::any const& argument) {
	if (!BaseMultiplayerManager::broadcast_packet(type, argument)) {
		return false;
	}

	OV_ERR_FAIL_COND_V(!type.can_client_send, false);
	return _send_packet_to_host<PacketTypes::broadcast_packet>(BroadcastData { type.packet_id, argument });
}

bool ClientManager::send_packet(client_id_type client_id, PacketType const& type, std::any const& argument) {
	if (!BaseMultiplayerManager::send_packet(client_id, type, argument)) {
		return false;
	}

	OV_ERR_FAIL_COND_V(!type.can_client_send, false);
	return _send_packet_to_host<PacketTypes::retransmit_packet>(RetransmitData { client_id, type.packet_id, argument });
}

int64_t ClientManager::poll() {
	int64_t result = client.available_packets();
	if (result < 1) {
		return result;
	}

	PacketSpan span = client.packet_span();
	if (client_id == INVALID_CLIENT_ID) {
		OV_ERR_FAIL_COND_V(client.get_current_sequence_value() != 0, -1);
		client_id = span.read<decltype(client_id)>();
		return poll();
	}

	decltype(PacketType::packet_id) packet_id = span.read<decltype(packet_id)>();
	OV_ERR_FAIL_COND_V(!PacketType::is_valid_type(packet_id), -1);

	PacketType const& packet_type = PacketType::get_type_by_id(packet_id);
	if (!packet_type.process_packet(this, span)) {
		// TODO: packet processing failed
		return -1;
	}
	// TODO: packet was processed

	return result;
}

bool ClientManager::add_packet_to_cache(std::span<uint8_t> bytes) {
	OV_ERR_FAIL_COND_V(bytes.size_bytes() > ReliableUdpClient::MAX_PACKET_SIZE, false);
	decltype(packet_cache)::iterator begin = packet_cache.append(bytes.data(), bytes.size_bytes());
	decltype(packet_cache)::iterator end = packet_cache.end();
	OV_ERR_FAIL_COND_V(begin == end, false);
	return sequence_to_index.try_emplace(client.get_next_sequence_value(), PacketCacheIndex { begin, end }).second;
}

std::vector<uint8_t> ClientManager::get_packet_cache(sequence_type sequence_value) {
	decltype(sequence_to_index)::iterator it = sequence_to_index.find(sequence_value);
	OV_ERR_FAIL_COND_V(it == sequence_to_index.end(), {});
	return { it.value().begin, it.value().end };
}

void ClientManager::remove_from_cache(sequence_type sequence_value) {
	decltype(sequence_to_index)::iterator it = sequence_to_index.find(sequence_value);
	OV_ERR_FAIL_COND(it == sequence_to_index.end());
	sequence_to_index.unordered_erase(it);
}
