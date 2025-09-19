#pragma once

#include <cstdint>

#include "openvic-simulation/multiplayer/Constants.hpp"
#include "openvic-simulation/multiplayer/HostSession.hpp"
#include "openvic-simulation/multiplayer/PacketType.hpp"
#include "openvic-simulation/multiplayer/lowlevel/Constants.hpp"
#include "openvic-simulation/types/RingBuffer.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GameManager;

	struct BaseMultiplayerManager {
		BaseMultiplayerManager(GameManager* game_manager = nullptr);
		virtual ~BaseMultiplayerManager() = default;

		using client_id_type = OpenVic::client_id_type;
		using sequence_type = reliable_udp_sequence_type;

		static constexpr client_id_type HOST_ID = MP_HOST_ID;

		virtual bool broadcast_packet(PacketType const& type, PacketType::argument_type argument);
		virtual bool send_packet(client_id_type client_id, PacketType const& type, PacketType::argument_type argument);
		virtual int64_t poll() = 0;
		virtual void close() = 0;

		enum class ConnectionType : uint8_t { HOST, CLIENT };

		virtual ConnectionType get_connection_type() const = 0;

		template<typename T>
		T* cast_as() {
			if (get_connection_type() == T::type_tag) {
				return static_cast<T*>(this);
			}
			return nullptr;
		}

		template<typename T>
		T const* cast_as() const {
			if (get_connection_type() == T::type_tag) {
				return static_cast<T const*>(this);
			}
			return nullptr;
		}

		PacketSpan get_last_raw_packet() {
			return last_raw_packet;
		}

	protected:
		bool PROPERTY_ACCESS(in_lobby, protected, false);
		GameManager* PROPERTY_PTR_ACCESS(game_manager, protected);
		HostSession PROPERTY_ACCESS(host_session, protected);

		RingBuffer<uint8_t> packet_cache;

		struct PacketCacheIndex {
			decltype(packet_cache)::const_iterator begin;
			decltype(packet_cache)::const_iterator end;
			constexpr bool is_valid() const {
				return begin != end;
			}
		};

		memory::vector<uint8_t> last_raw_packet;

		friend bool PacketTypes::send_raw_packet_process_callback( //
			BaseMultiplayerManager* multiplayer_manager, PacketSpan packet
		);
	};
}
