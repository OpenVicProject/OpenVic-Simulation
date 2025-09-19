#pragma once

#include <concepts>
#include <cstdint>
#include <string_view>

#include "openvic-simulation/multiplayer/lowlevel/Constants.hpp"
#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkResolver.hpp"
#include "openvic-simulation/types/EnumBitfield.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct NetworkSocketBase {
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

		using port_type = socket_port_type;

		virtual ~NetworkSocketBase() = default;

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

		memory::unique_base_ptr<NetworkSocketBase> accept(IpAddress& r_ip, port_type& r_port) {
			return memory::unique_base_ptr<NetworkSocketBase> { static_cast<NetworkSocketBase*>(_accept(r_ip, r_port)) };
		}

		template<std::derived_from<NetworkSocketBase> T>
		memory::unique_ptr<T> accept_as(IpAddress& r_ip, port_type& r_port) {
			return memory::unique_ptr<T> { static_cast<T*>(_accept(r_ip, r_port)) };
		}

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

		virtual NetworkResolver::Provider provider() const = 0;

	protected:
		virtual NetworkSocketBase* _accept(IpAddress& r_ip, port_type& r_port) = 0;
	};

	template<>
	struct enable_bitfield<NetworkSocketBase::PollType> : std::true_type {};
}
