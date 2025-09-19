#include "HostManager.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <future>
#include <system_error>
#include <thread>

#include "openvic-simulation/multiplayer/BaseMultiplayerManager.hpp"
#include "openvic-simulation/multiplayer/ClientManager.hpp"
#include "openvic-simulation/multiplayer/HostSession.hpp"
#include "openvic-simulation/multiplayer/PacketType.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketReaderAdapter.hpp"
#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpClient.hpp"
#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpServer.hpp"
#include "openvic-simulation/multiplayer/lowlevel/TcpPacketStream.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/TslHelper.hpp"

using namespace OpenVic;

HostManager::HostManager(GameManager* game_manager) : BaseMultiplayerManager(game_manager) {
	host_session.session_changed.connect(&HostManager::_on_session_changed, this);
}

HostManager::~HostManager() {
	this->disconnect_all();
}

void HostManager::_on_session_changed() {
	broadcast_packet(PacketTypes::update_host_session, &host_session);
}

bool HostManager::listen(NetworkSocket::port_type port, HostnameAddress const& bind_address) {
	return server.listen_to(port, bind_address.resolved_address()) == NetworkError::OK;
}

bool HostManager::broadcast_packet(PacketType const& type, PacketType::argument_type argument) {
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

bool HostManager::send_packet(client_id_type client_id, PacketType const& type, PacketType::argument_type argument) {
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

void HostManager::close() {
	server.close();
}

size_t HostManager::send_resource(std::string_view path, bool recursive) {
	if (resource_clients.empty()) {
		resource_clients.reserve(clients.size());
		while (resource_server.is_connection_available()) {
			resource_clients.push_back(resource_server.take_packet_stream());
			OV_ERR_CONTINUE(resource_clients.back()->get_status() != TcpPacketStream::Status::CONNECTED);
		}
	}

	std::error_code ec;
	std::filesystem::path filepath = std::filesystem::current_path(ec);
	OV_ERR_FAIL_COND_V(!ec, 0);

	filepath /= path;
	while (std::filesystem::is_symlink(filepath, ec) && ec == std::error_code {}) {
		filepath = std::filesystem::read_symlink(filepath, ec);
		OV_ERR_FAIL_COND_V(!ec, 0);
	}
	OV_ERR_FAIL_COND_V(!ec, 0);

	const bool path_exists = std::filesystem::exists(filepath, ec);
	OV_ERR_FAIL_COND_V(!ec, 0);
	OV_ERR_FAIL_COND_V(!path_exists, 0);

	memory::vector<uint8_t> file_bytes;
	auto send_to_clients = [&](std::filesystem::directory_entry const& entry) -> size_t {
		uintmax_t size = entry.file_size(ec);
		OV_ERR_FAIL_COND_V(!ec, 0);

		file_bytes.resize(size);
		{
			std::FILE* file = utility::fopen(entry.path().c_str(), "rb");
			OV_ERR_FAIL_COND_V(file == nullptr, 0);

			size_t read_count = std::fread(file_bytes.data(), sizeof(uint8_t), size, file);
			std::fclose(file);
			OV_ERR_FAIL_COND_V(read_count != size, 0);
		}

		for (memory::unique_ptr<TcpPacketStream>& client : resource_clients) {
			OV_ERR_CONTINUE(client->set_data(file_bytes) != NetworkError::OK);
		}
		return 1;
	};

	if (std::filesystem::is_regular_file(filepath, ec)) {
		std::filesystem::directory_entry const& entry { filepath, ec };
		OV_ERR_FAIL_COND_V(!ec, 0);
		return send_to_clients(entry);
	} else {
		OV_ERR_FAIL_COND_V(!ec, 0);
	}

	size_t file_count = 0;
	for (std::filesystem::recursive_directory_iterator it { filepath, ec }; it != decltype(it)(); ++it) {
		std::filesystem::directory_entry const& entry = *it;
		OV_ERR_CONTINUE(!ec);

		if (entry.is_directory(ec)) {
			if (!recursive) {
				it.disable_recursion_pending();
			}
			continue;
		} else {
			OV_ERR_CONTINUE(!ec);
		}

		if (!entry.is_regular_file(ec)) {
			Logger::warning("Only regular files can be sent as a resource, '", filepath, "' is not a regular file.");
			continue;
		} else {
			OV_ERR_CONTINUE(!ec);
		}

		const size_t sent_files = send_to_clients(entry);
		OV_ERR_CONTINUE(sent_files == 0);
		file_count += sent_files;
	}
	return file_count;
}

std::future<bool> HostManager::check_for_resource(client_id_type client_id, memory::string const& check_path) {
	OV_ERR_FAIL_INDEX_V(client_id, resource_clients.size(), {});

	using namespace std::chrono_literals;
	static constexpr std::chrono::duration SLEEP_DURATION = 1000us;

	TcpPacketStream& client = *resource_clients[client_id];

	PacketBuilder<> pb;
	pb.put_back(check_path);
	pb.put_back(true);
	client.set_data(pb);

	return std::async(std::launch::async, [&]() -> bool {
		while (client.poll() == NetworkError::OK && client.get_status() != TcpPacketStream::Status::CONNECTED) {
			std::this_thread::sleep_for(SLEEP_DURATION);
		}

		if (client.get_status() != TcpPacketStream::Status::CONNECTED) {
			return false;
		}

		// TODO: support multiple simultaneous resource checks

		std::string_view str;
		PacketBuffer buffer;
		buffer.resize(sizeof(uint64_t));
		while (str != check_path) {
			if (client.poll() == NetworkError::OK) {
				std::this_thread::sleep_for(SLEEP_DURATION);
				continue;
			} else if (client.get_status() != TcpPacketStream::Status::CONNECTED) {
				return false;
			}

			client.get_data(buffer);
			uint64_t buffer_size = buffer.read<uint64_t>();
			if (buffer_size > buffer.size()) {
				buffer.resize(buffer_size);
				client.get_data(buffer);
			} else {
				client.get_data({ buffer.data(), buffer_size });
			}
			str = buffer.read<std::string_view>();
		}

		return buffer.read<bool>();
	});
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

memory::vector<uint8_t> HostManager::get_packet_cache_for(client_id_type client_id, sequence_type sequence_value) {
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
