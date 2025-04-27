#pragma once

#include <charconv>
#include <cstring>

#include <reliable.h>

#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpClient.hpp"
#include "openvic-simulation/multiplayer/lowlevel/UdpServer.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	struct ReliableUdpServer : UdpServer {
		void set_client_config_template(reliable_config_t const& config) {
			_client_config_template = config;
		}

		reliable_config_t const& get_client_config_template() const {
			return _client_config_template;
		}

	protected:
		inline ReliableUdpClient* _take_next_client() override {
			return static_cast<ReliableUdpClient*>(UdpServer::_take_next_client());
		}

		inline ReliableUdpClient* _create_client() override {
			reliable_config_t config = _client_config_template;
			config.id = _clients.size() + _pending_clients.size() + 1;

			size_t name_length = std::strlen(config.name);
			if (OV_unlikely(name_length >= 225)) {
				static constexpr const char server_connection[] = "server-connection-";
				static_assert(sizeof(server_connection) - 1 < sizeof(config.name));

				name_length = sizeof(server_connection) - 1;
				std::strncpy(config.name, server_connection, name_length);
			}

			std::to_chars(config.name + name_length, config.name + sizeof(config.name), config.id);

			return new ReliableUdpClient(std::move(config));
		}

		reliable_config_t _client_config_template { .name = "server-connection-" };
	};
}
