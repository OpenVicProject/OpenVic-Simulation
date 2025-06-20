#pragma once

#if !(defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#error "UnixSocket.hpp should only be included on unix systems"
#endif

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkResolver.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocketBase.hpp"

struct sockaddr_storage;

namespace OpenVic {
	struct UnixSocket final : NetworkSocketBase {
		static constexpr NetworkResolver::Provider provider_value = NetworkResolver::Provider::UNIX;

		UnixSocket();
		~UnixSocket() override;

		NetworkError open(Type p_type, NetworkResolver::Type& ip_type) override;
		void close() override;
		NetworkError bind(IpAddress const& p_addr, port_type p_port) override;
		NetworkError listen(int p_max_pending) override;
		NetworkError connect_to_host(IpAddress const& p_addr, port_type p_port) override;
		NetworkError poll(PollType p_type, int timeout) const override;
		NetworkError receive(uint8_t* p_buffer, size_t p_len, int64_t& r_read) override;
		NetworkError receive_from( //
			uint8_t* p_buffer, size_t p_len, int64_t& r_read, IpAddress& r_ip, port_type& r_port, bool p_peek = false
		) override;
		NetworkError send(const uint8_t* p_buffer, size_t p_len, int64_t& r_sent) override;
		NetworkError
		send_to(const uint8_t* p_buffer, size_t p_len, int64_t& r_sent, IpAddress const& p_ip, port_type p_port) override;

		bool is_open() const override;
		int available_bytes() const override;
		NetworkError get_socket_address(IpAddress* r_ip, port_type* r_port) const override;

		// Returns OK if the socket option has been set successfully
		NetworkError set_broadcasting_enabled(bool p_enabled) override;
		void set_blocking_enabled(bool p_enabled) override;
		void set_ipv6_only_enabled(bool p_enabled) override;
		void set_tcp_no_delay_enabled(bool p_enabled) override;
		void set_reuse_address_enabled(bool p_enabled) override;
		NetworkError change_multicast_group(IpAddress const& p_multi_address, std::string_view p_if_name, bool add) override;

		NetworkResolver::Provider provider() const override {
			return provider_value;
		}

		static void setup();
		static void cleanup();

	protected:
		UnixSocket* _accept(IpAddress& r_ip, port_type& r_port) override;

	private:
		static size_t _set_addr_storage( //
			sockaddr_storage* p_addr, IpAddress const& p_ip, port_type p_port, NetworkResolver::Type p_ip_type
		);
		static void _set_ip_port(sockaddr_storage* addr, IpAddress* r_ip, port_type* r_port);

		NetworkError _get_socket_error() const;
		bool _can_use_ip(IpAddress const& p_ip, const bool p_for_bind) const;
		void _set_close_exec_enabled(bool p_enabled);

		int32_t _sock = -1;
		NetworkResolver::Type _ip_type = NetworkResolver::Type::NONE;
		bool _is_stream = false;
	};
}
