#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "openvic-simulation/multiplayer/IpAddress.hpp"
#include "openvic-simulation/multiplayer/NetworkError.hpp"
#include "openvic-simulation/multiplayer/NetworkResolver.hpp"
#include "openvic-simulation/types/EnumBitfield.hpp"

namespace OpenVic {
	struct NetworkSocketBase {
		enum class Provider : uint8_t {
			UNIX,
			WINDOWS,
		};

		enum class PollType : uint8_t {
			IN = 1 << 0,
			OUT = 1 << 1,
			IN_OUT = IN | OUT,
		};

		enum class Type : uint8_t {
			NONE,
			TCP,
			UDP,
		};

		using port_type = uint16_t;

		virtual ~NetworkSocketBase() {}

		virtual NetworkError open(Type p_type, NetworkResolver::Type& ip_type) = 0;
		virtual void close() = 0;
		virtual NetworkError bind(IpAddress const& p_addr, port_type p_port) = 0;
		virtual NetworkError listen(int p_max_pending) = 0;
		virtual NetworkError connect_to_host(IpAddress const& p_addr, port_type p_port) = 0;
		virtual NetworkError poll(PollType p_type, int timeout) const = 0;
		virtual NetworkError receive(uint8_t* p_buffer, size_t p_len, int64_t& r_read) = 0;
		virtual NetworkError receive_from( //
			uint8_t* p_buffer, size_t p_len, int64_t& r_read, IpAddress& r_ip, port_type& r_port, bool p_peek = false
		) = 0;
		virtual NetworkError send(const uint8_t* p_buffer, size_t p_len, int64_t& r_sent) = 0;
		virtual NetworkError send_to( //
			const uint8_t* p_buffer, size_t p_len, int64_t& r_sent, IpAddress const& p_ip, port_type p_port
		) = 0;
		virtual std::unique_ptr<NetworkSocketBase> virtual_accept(IpAddress& r_ip, port_type& r_port) = 0;

		virtual bool is_open() const = 0;
		virtual int available_bytes() const = 0;
		virtual NetworkError get_socket_address(IpAddress* r_ip, uint16_t* r_port) const = 0;

		// Returns OK if the socket option has been set successfully
		virtual NetworkError set_broadcasting_enabled(bool p_enabled) = 0;
		virtual void set_blocking_enabled(bool p_enabled) = 0;
		virtual void set_ipv6_only_enabled(bool p_enabled) = 0;
		virtual void set_tcp_no_delay_enabled(bool p_enabled) = 0;
		virtual void set_reuse_address_enabled(bool p_enabled) = 0;
		virtual NetworkError change_multicast_group(IpAddress const& p_multi_address, std::string_view p_if_name, bool add) = 0;

		virtual Provider provider() const = 0;
	};

	template<>
	struct enable_bitfield<NetworkSocketBase::PollType> : std::true_type {};
}
