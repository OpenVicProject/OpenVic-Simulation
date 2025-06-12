#pragma once

#include <future>
#include <limits>
#include <optional>

#include "openvic-simulation/multiplayer/BaseMultiplayerManager.hpp"
#include "openvic-simulation/multiplayer/lowlevel/HostnameAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpClient.hpp"
#include "openvic-simulation/multiplayer/lowlevel/TcpPacketStream.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct GameManager;

	struct ClientManager final : BaseMultiplayerManager {
		using BaseMultiplayerManager::BaseMultiplayerManager;

		bool connect_to(HostnameAddress const& address, NetworkSocket::port_type port);

		bool broadcast_packet(PacketType const& type, PacketType::argument_type argument) override;
		bool send_packet(client_id_type client_id, PacketType const& type, PacketType::argument_type argument) override;
		int64_t poll() override;
		void close() override;

		bool connect_to_resource_server(std::optional<NetworkSocket::port_type> port = std::nullopt);
		std::future<void> poll_resource_server();

		bool is_running_as_host() const;

		static constexpr ConnectionType type_tag = ConnectionType::CLIENT;
		inline constexpr ConnectionType get_connection_type() const override {
			return type_tag;
		}

		static constexpr client_id_type INVALID_CLIENT_ID = std::numeric_limits<client_id_type>::max() - 1;

	private:
		ReliableUdpClient PROPERTY_REF(client);
		TcpPacketStream resource_client;
		client_id_type PROPERTY(client_id, INVALID_CLIENT_ID);

		friend bool PacketTypes::update_host_session_process_callback(BaseMultiplayerManager* game_manager, PacketSpan packet);

		ordered_map<sequence_type, PacketCacheIndex> sequence_to_index;

		bool add_packet_to_cache(std::span<uint8_t> bytes);
		memory::vector<uint8_t> get_packet_cache(sequence_type sequence_value);
		void remove_from_cache(sequence_type sequence_value);

		template<PacketType const& type>
		bool _send_packet_to_host(auto const& argument);
	};
}
