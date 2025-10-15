#include "ClientManager.hpp"

#include <cstdio>
#include <filesystem>
#include <future>
#include <string_view>
#include <system_error>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/multiplayer/PacketType.hpp"
#include "openvic-simulation/multiplayer/lowlevel/HostnameAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"
#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpClient.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/Marshal.hpp"

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
	if constexpr (requires { type.create_packet(this, argument, builder); }) {
		if (!type.create_packet(this, argument, builder)) {
			return false;
		}
	} else {
		if (!type.create_packet(this, &argument, builder)) {
			return false;
		}
	}

	if (!add_packet_to_cache(builder)) {
		return false;
	}

	client.set_packet(builder);
	return true;
}

bool ClientManager::broadcast_packet(PacketType const& type, PacketType::argument_type argument) {
	if (!BaseMultiplayerManager::broadcast_packet(type, argument)) {
		return false;
	}

	OV_ERR_FAIL_COND_V(!type.can_client_send, false);
	return _send_packet_to_host<PacketTypes::broadcast_packet>(BroadcastData { type.packet_id, argument });
}

bool ClientManager::send_packet(client_id_type client_id, PacketType const& type, PacketType::argument_type argument) {
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
	if (player == nullptr) {
		OV_ERR_FAIL_COND_V(client.get_current_sequence_value() != 0, -1);
		client_id_type client_id = span.read<decltype(client_id)>();
		OV_ERR_FAIL_COND_V(client_id == INVALID_CLIENT_ID, -1);
		player = host_session.add_player(client_id, "");
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

void ClientManager::close() {
	client.close();
}

bool ClientManager::connect_to_resource_server(std::optional<NetworkSocket::port_type> port) {
	if (is_running_as_host()) {
		return true;
	}

	NetworkError err = resource_client.connect_to(client.get_peer_ip(), port.value_or(client.get_peer_port() + 1));
	OV_ERR_FAIL_COND_V(err != NetworkError::OK, false);
	return true;
}

std::future<void> ClientManager::poll_resource_server() {
	if (is_running_as_host()) {
		return {};
	}

	if (resource_client.poll() != NetworkError::OK || resource_client.get_status() != TcpPacketStream::Status::CONNECTED) {
		return {};
	}

	PacketBuffer buffer = resource_client.packet_buffer(sizeof(size_t));
	size_t buffer_size = buffer.read<size_t>();
	buffer.clear();
	buffer.resize(buffer_size);
	resource_client.get_data(buffer);
	std::string_view str = buffer.read<std::string_view>();
	std::filesystem::path path = str;

	bool is_check = buffer.read<bool>();
	if (is_check) {
		return std::async(std::launch::async, [&] {
			std::error_code ec;

			PacketBuilder<> pb;
			pb.put_back<uint64_t>(utility::encode(str) + utility::encode(true));
			pb.put_back(str);

			if (!std::filesystem::is_directory(path.parent_path(), ec)) {
				pb.put_back(false);
				OV_ERR_FAIL_COND(!ec);
				return;
			}

			// TODO: get checksum of file
			if (!std::filesystem::is_regular_file(path, ec)) {
				pb.put_back(false);
				OV_ERR_FAIL_COND(!ec);
				return;
			}

			pb.put_back(true);
		});
	}

	size_t index = buffer.index();
	std::span<const uint8_t> data { &buffer.data()[index], buffer.size() - index };

	return std::async(std::launch::async, [&] {
		std::error_code ec;

		OV_ERR_FAIL_COND(!std::filesystem::create_directories(path.parent_path(), ec) || !ec);

		{
			std::FILE* file = utility::fopen(path.c_str(), "wb");
			OV_ERR_FAIL_COND(file == nullptr);

			size_t write_count = std::fwrite(data.data(), sizeof(uint8_t), data.size(), file);
			std::fclose(file);
			OV_ERR_FAIL_COND(write_count != buffer_size);
		}
	});
}

bool ClientManager::is_running_as_host() const {
	return game_manager->get_host_manager() != nullptr;
}

bool ClientManager::add_packet_to_cache(std::span<uint8_t> bytes) {
	OV_ERR_FAIL_COND_V(bytes.size_bytes() > ReliableUdpClient::MAX_PACKET_SIZE, false);
	decltype(packet_cache)::iterator begin = packet_cache.append(bytes.data(), bytes.size_bytes());
	decltype(packet_cache)::iterator end = packet_cache.end();
	OV_ERR_FAIL_COND_V(begin == end, false);
	return sequence_to_index.try_emplace(client.get_next_sequence_value(), PacketCacheIndex { begin, end }).second;
}

memory::vector<uint8_t> ClientManager::get_packet_cache(sequence_type sequence_value) {
	decltype(sequence_to_index)::iterator it = sequence_to_index.find(sequence_value);
	OV_ERR_FAIL_COND_V(it == sequence_to_index.end(), {});
	return { it.value().begin, it.value().end };
}

void ClientManager::remove_from_cache(sequence_type sequence_value) {
	decltype(sequence_to_index)::iterator it = sequence_to_index.find(sequence_value);
	OV_ERR_FAIL_COND(it == sequence_to_index.end());
	sequence_to_index.unordered_erase(it);
}
