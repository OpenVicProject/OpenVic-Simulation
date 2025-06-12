#pragma once

#include <cstdint>
#include <memory>
#include <type_traits>

#include <tsl/ordered_map.h>

#include "openvic-simulation/multiplayer/BaseMultiplayerManager.hpp"
#include "openvic-simulation/multiplayer/HostSession.hpp"
#include "openvic-simulation/multiplayer/lowlevel/HostnameAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpClient.hpp"
#include "openvic-simulation/multiplayer/lowlevel/ReliableUdpServer.hpp"
#include "openvic-simulation/multiplayer/lowlevel/TcpServer.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct HostManager final : BaseMultiplayerManager {
		using BaseMultiplayerManager::BaseMultiplayerManager;

		bool listen(NetworkSocket::port_type port, HostnameAddress const& bind_address = IpAddress { "*" });

		bool broadcast_packet(PacketType const& type, std::any const& argument) override;
		bool send_packet(client_id_type client_id, PacketType const& type, std::any const& argument) override;
		int64_t poll() override;

		static constexpr ConnectionType type_tag = ConnectionType::HOST;
		inline constexpr ConnectionType get_connection_type() const override {
			return type_tag;
		}

		bool broadcast_data(std::span<uint8_t> bytes);
		bool send_data(client_id_type client_id, std::span<uint8_t> bytes);

		HostSession& get_host_session();

		ReliableUdpClient* get_client_by_id(client_id_type client_id);
		ReliableUdpClient const* get_client_by_id(client_id_type client_id) const;

		template<bool IsConst>
		struct ClientIterable;

		ClientIterable<false> client_iterable();
		ClientIterable<true> client_iterable() const;

	private:
		ReliableUdpServer PROPERTY_REF(server);
		TcpServer file_server;

		struct ClientValue {
			std::unique_ptr<ReliableUdpClient> client;
			ordered_map<sequence_type, PacketCacheIndex> sequence_to_index;

			PacketCacheIndex add_cache_index(PacketCacheIndex const& index);
			void remove_packet(sequence_type sequence_value);
		};
		ordered_map<client_id_type, ClientValue> clients;

		HostManager::PacketCacheIndex add_packet_to_cache(std::span<uint8_t> bytes);

		HostManager::PacketCacheIndex cache_packet_for(client_id_type client_id, std::span<uint8_t> bytes);
		std::vector<uint8_t> get_packet_cache_for(client_id_type client_id, sequence_type sequence_value);
		void remove_from_cache_for(client_id_type client_id, sequence_type sequence_value);

	public:
		template<bool IsConst>
		struct ClientIterable {
			class iterator final
				: protected std::conditional_t<IsConst, decltype(clients)::const_iterator, decltype(clients)::iterator> {
				friend struct ClientIterable<IsConst>;
				using base_type = std::conditional_t<IsConst, decltype(clients)::const_iterator, decltype(clients)::iterator>;

				iterator(const base_type& other) noexcept : base_type(other) {}

			public:
				using client_pointer = std::conditional_t<
					IsConst, const decltype(decltype(clients)::value_type::second_type::client)::pointer,
					decltype(decltype(clients)::value_type::second_type::client)::pointer>;

				using pair_type = std::pair<decltype(clients)::key_type, client_pointer>;

				using difference_type = decltype(clients)::difference_type;

				iterator() noexcept {}

				template<bool OtherConst = IsConst>
				requires OtherConst
				iterator(const ClientIterable<!OtherConst>::iterator& other) noexcept : base_type(other) {}

				iterator(const iterator& other) = default;
				iterator(iterator&& other) = default;
				iterator& operator=(const iterator& other) = default;
				iterator& operator=(iterator&& other) = default;

				iterator operator++() {
					++(*this);
					return *this;
				}

				iterator operator--() {
					--(*this);
					return *this;
				}

				iterator operator++(int) {
					iterator tmp { *this };
					++(*this);
					return tmp;
				}

				iterator operator--(int) {
					iterator tmp { *this };
					--(*this);
					return tmp;
				}

				pair_type::first_type key() const {
					return base_type::key();
				}

				pair_type::second_type value() const {
					return base_type::value().client.get();
				}

				pair_type operator*() const {
					return { key(), value() };
				}

				client_pointer operator->() const {
					return value();
				}

				pair_type operator[](difference_type n) const {
					return *(*this + n);
				}

				iterator& operator+=(difference_type n) {
					*this += n;
					return *this;
				}

				iterator& operator-=(difference_type n) {
					*this -= n;
					return *this;
				}

				iterator operator+(difference_type n) const {
					iterator tmp { *this };
					tmp += n;
					return tmp;
				}

				iterator operator-(difference_type n) const {
					iterator tmp { *this };
					tmp -= n;
					return tmp;
				}

				friend iterator operator+(difference_type lhs, iterator const& rhs) {
					return rhs + lhs;
				}

				difference_type operator-(iterator const& rhs) const {
					return *this - rhs;
				}

				bool operator==(iterator const& rhs) const {
					return *this == rhs;
				}

				auto operator<=>(iterator const& rhs) const {
					return *this <=> rhs;
				}
			};

			iterator begin() const {
				return iterator { source.begin() };
			}

			iterator end() const {
				return iterator { source.end() };
			}

			size_t size() const {
				return source.size();
			}

		private:
			friend struct HostManager;
			std::conditional_t<IsConst, decltype(clients) const&, decltype(clients)&> source;
			ClientIterable(decltype(source) source) : source { source } {}
		};
	};
}
