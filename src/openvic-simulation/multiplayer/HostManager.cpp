
#include "HostManager.hpp"

#include "openvic-simulation/multiplayer/BaseMultiplayerManager.hpp"
#include "openvic-simulation/multiplayer/ClientManager.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"
#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpClient.hpp"
#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpServer.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/TslHelper.hpp"

#include "types/OrderedContainers.hpp"

using namespace OpenVic;

bool HostManager::listen(NetworkSocket::port_type port, HostnameAddress const& bind_address) {
	return server.listen_to(port, bind_address.resolved_address()) == NetworkError::OK;
}

bool HostManager::broadcast_packet(PacketType const& type, std::any const& argument) {
	if (!BaseMultiplayerManager::broadcast_packet(type, argument)) {
		return false;
	}

	PacketBuilder builder;
	builder.put_back(type.packet_id);
	if (!type.create_packet(this, argument, builder)) {
		return false;
	}

	return broadcast_data(builder);
}

bool HostManager::send_packet(client_id_type client_id, PacketType const& type, std::any const& argument) {
	if (!BaseMultiplayerManager::send_packet(client_id, type, argument)) {
		return false;
	}

	decltype(clients)::iterator it = clients.find(client_id);
	OV_ERR_FAIL_COND_V(it == clients.end(), false);

	PacketBuilder builder;
	builder.put_back(type.packet_id);

	if (!type.create_packet(this, argument, builder)) {
		return false;
	}

	PacketCacheIndex index = cache_packet_for(client_id, builder);
	if (!index.is_valid()) {
		return false;
	}

	if (it.value().client->set_packet(builder) != NetworkError::OK) {
		return false;
	}

	return true;
}

int64_t HostManager::poll() {
	if (server.poll() != NetworkError::OK && !server.is_connection_available()) {
		return -1;
	}

	{
		PacketBuilder builder;
		while (server.is_connection_available()) {
			size_t client_id = clients.size();
			clients.try_emplace(client_id, ClientValue { server.take_next_client_as<ReliableUdpClient>() });
			OV_ERR_CONTINUE(clients.back().second.client->packet_span().read<uint8_t>() != 1);
			builder.put_back<decltype(ClientManager::INVALID_CLIENT_ID)>(client_id);
			clients.back().second.client->set_packet(builder);
			builder.clear();
		}
	}

	int64_t result = 0;
	for (std::pair<client_id_type, ClientValue> const& pair : clients) {
		ReliableUdpClient& client = *pair.second.client;

		while (client.available_packets() > 0) {
			PacketSpan span = client.packet_span();
			decltype(PacketType::packet_id) packet_id;
			packet_id = span.read<decltype(packet_id)>();
			OV_ERR_CONTINUE(!PacketType::is_valid_type(packet_id));

			PacketType const& packet_type = PacketType::get_type_by_id(packet_id);
			if (!packet_type.process_packet(this, span)) {
				// TODO: packet processing failed
				continue;
			}
			result += 1;
			// TODO: packet was processed
		}
	}
	return result;
}

bool HostManager::broadcast_data(std::span<uint8_t> bytes) {
	PacketCacheIndex index = add_packet_to_cache(bytes);

	if (!index.is_valid()) {
		return false;
	}

	bool has_succeeded = true;
	for (std::pair<client_id_type const&, ClientValue&> pair : mutable_iterator(clients)) {
		ReliableUdpClient& client = *pair.second.client;
		if (client.set_packet(bytes) == NetworkError::OK) {
			has_succeeded &= pair.second.add_cache_index(index).is_valid();
			continue;
		}
		has_succeeded = false;
	}
	return has_succeeded;
}

bool HostManager::send_data(client_id_type client_id, std::span<uint8_t> bytes) {
	decltype(clients)::iterator it = clients.find(client_id);
	OV_ERR_FAIL_COND_V(it == clients.end(), false);

	PacketCacheIndex index = cache_packet_for(client_id, bytes);
	if (index.is_valid()) {
		return false;
	}

	if (it.value().client->set_packet(bytes) != NetworkError::OK) {
		return false;
	}
	return true;
}

HostSession& HostManager::get_host_session() {
	return host_session;
}

ReliableUdpClient* HostManager::get_client_by_id(client_id_type client_id) {
	decltype(clients)::iterator it = clients.find(client_id);
	OV_ERR_FAIL_COND_V(it == clients.end(), nullptr);
	return it.value().client.get();
}

ReliableUdpClient const* HostManager::get_client_by_id(client_id_type client_id) const {
	decltype(clients)::const_iterator it = clients.find(client_id);
	OV_ERR_FAIL_COND_V(it == clients.end(), nullptr);
	return it.value().client.get();
}

HostManager::ClientIterable<false> HostManager::client_iterable() {
	return ClientIterable<false> { clients };
}

HostManager::ClientIterable<true> HostManager::client_iterable() const {
	return ClientIterable<true> { clients };
}

HostManager::PacketCacheIndex HostManager::ClientValue::add_cache_index(PacketCacheIndex const& index) {
	OV_ERR_FAIL_COND_V(!index.is_valid(), index);

	std::pair<decltype(sequence_to_index)::iterator, bool> emplace_result =
		sequence_to_index.try_emplace(client->get_next_sequence_value(), index);

	OV_ERR_FAIL_COND_V(!emplace_result.second, (PacketCacheIndex { index.end, index.end }));
	return emplace_result.first.value();
}

void HostManager::ClientValue::remove_packet(sequence_type sequence_value) {
	decltype(sequence_to_index)::iterator it = sequence_to_index.find(sequence_value);
	OV_ERR_FAIL_COND(it == sequence_to_index.end());
	sequence_to_index.unordered_erase(it);
}

HostManager::PacketCacheIndex HostManager::cache_packet_for(client_id_type client_id, std::span<uint8_t> bytes) {
	decltype(clients)::iterator it = clients.find(client_id);
	OV_ERR_FAIL_COND_V(it == clients.end(), (PacketCacheIndex { packet_cache.end(), packet_cache.end() }));

	PacketCacheIndex index = add_packet_to_cache(bytes);
	OV_ERR_FAIL_COND_V(!index.is_valid(), index);

	return it.value().add_cache_index(index);
}

HostManager::PacketCacheIndex HostManager::add_packet_to_cache(std::span<uint8_t> bytes) {
	OV_ERR_FAIL_COND_V(
		bytes.size_bytes() > ReliableUdpClient::MAX_PACKET_SIZE, (PacketCacheIndex { packet_cache.end(), packet_cache.end() })
	);

	decltype(packet_cache)::iterator begin = packet_cache.append(bytes.data(), bytes.size_bytes());
	decltype(packet_cache)::iterator end = packet_cache.end();
	return { begin, end };
}

std::vector<uint8_t> HostManager::get_packet_cache_for(client_id_type client_id, sequence_type sequence_value) {
	decltype(clients)::iterator client_it = clients.find(client_id);
	OV_ERR_FAIL_COND_V(client_it == clients.end(), {});

	decltype(client_it.value().sequence_to_index)::iterator it = client_it.value().sequence_to_index.find(sequence_value);
	OV_ERR_FAIL_COND_V(it == client_it.value().sequence_to_index.end(), {});
	return { it.value().begin, it.value().end };
}

void HostManager::remove_from_cache_for(client_id_type client_id, sequence_type sequence_value) {
	decltype(clients)::iterator it = clients.find(client_id);
	OV_ERR_FAIL_COND(it == clients.end());

	it.value().remove_packet(sequence_value);
}
